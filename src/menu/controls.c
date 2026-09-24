#include "controls.h"

#include "audio/soundplayer.h"
#include "font/dejavu_sans.h"
#include "font/font.h"
#include "strings/translations.h"
#include "system/controller.h"
#include "system/display.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/strings/strings.h"

enum ControllerButtonIcon {
    ControllerButtonIconA,
    ControllerButtonIconB,
    ControllerButtonIconS,

    ControllerButtonIconCU,
    ControllerButtonIconCR,
    ControllerButtonIconCD,
    ControllerButtonIconCL,

    ControllerButtonIconDU,
    ControllerButtonIconDR,
    ControllerButtonIconDD,
    ControllerButtonIconDL,

    ControllerButtonIconZ,
    ControllerButtonIconR,
    ControllerButtonIconL,
};

enum ControllerDirectionIcon {
    ControllerDirectionIconC,
    ControllerDirectionIconD,
    ControllerDirectionIconJ,
};

#ifdef PSP
// assets/images/psp/button_icons.png: a 4x4 grid of 16 pixel cells. Each N64
// input shows the PSP button that presses it (see
// src/system/psp/controller_psp.c).
static struct ControllerIcon sControllerButtonIcons[] = {
    [ControllerButtonIconA]    = {48, 0, 16, 16},    // cross
    [ControllerButtonIconB]    = {32, 0, 16, 16},    // circle
    [ControllerButtonIconS]    = {0, 48, 16, 16},    // START

    [ControllerButtonIconCU]   = {0, 0, 16, 16},     // triangle
    [ControllerButtonIconCR]   = {32, 0, 16, 16},    // circle
    [ControllerButtonIconCD]   = {48, 0, 16, 16},    // cross
    [ControllerButtonIconCL]   = {16, 0, 16, 16},    // square

    [ControllerButtonIconDU]   = {0, 32, 16, 16},
    [ControllerButtonIconDR]   = {48, 32, 16, 16},
    [ControllerButtonIconDD]   = {32, 32, 16, 16},
    [ControllerButtonIconDL]   = {16, 32, 16, 16},

    [ControllerButtonIconZ]    = {16, 48, 16, 16},   // SELECT
    [ControllerButtonIconR]    = {48, 48, 16, 16},
    [ControllerButtonIconL]    = {32, 48, 16, 16},
};

static struct ControllerIcon sControllerDirectionIcons[] = {
    [ControllerDirectionIconC] = {0, 16, 16, 16},    // the four face buttons
    [ControllerDirectionIconD] = {16, 16, 16, 16},   // the D-pad
    [ControllerDirectionIconJ] = {32, 16, 16, 16},   // the analog nub
};

static struct ControllerIcon sControllerIndexIcons[] = {
    { 48, 18, 8, 12 },
    { 56, 18, 8, 12 },
};
#else
static struct ControllerIcon sControllerButtonIcons[] = {
    [ControllerButtonIconA]    = {0, 0, 12, 12},
    [ControllerButtonIconB]    = {12, 0, 12, 12},
    [ControllerButtonIconS]    = {24, 0, 12, 12},

    [ControllerButtonIconCU]   = {0, 24, 12, 12},
    [ControllerButtonIconCR]   = {12, 24, 12, 12},
    [ControllerButtonIconCD]   = {24, 24, 12, 12},
    [ControllerButtonIconCL]   = {36, 24, 12, 12},

    [ControllerButtonIconDU]   = {0, 36, 12, 12},
    [ControllerButtonIconDR]   = {12, 36, 12, 12},
    [ControllerButtonIconDD]   = {24, 36, 12, 12},
    [ControllerButtonIconDL]   = {36, 36, 12, 12},

    [ControllerButtonIconZ]    = {0, 48, 12, 12},
    [ControllerButtonIconR]    = {12, 48, 12, 12},
    [ControllerButtonIconL]    = {24, 48, 12, 12},
};

static struct ControllerIcon sControllerDirectionIcons[] = {
    [ControllerDirectionIconC] = {0, 12, 14, 12},
    [ControllerDirectionIconD] = {14, 12, 12, 12},
    [ControllerDirectionIconJ] = {26, 12, 15, 12},
};

static struct ControllerIcon sControllerIndexIcons[] = {
    { 54, 0, 5, 7 },
    { 59, 0, 5, 7 },
};
#endif

static uint8_t sControllerActionInputToButtonIcon[] = {
    [ControllerActionInputAButton]      = ControllerButtonIconA,
    [ControllerActionInputBButton]      = ControllerButtonIconB,
    [ControllerActionInputStartButton]  = ControllerButtonIconS,

    [ControllerActionInputCUpButton]    = ControllerButtonIconCU,
    [ControllerActionInputCRightButton] = ControllerButtonIconCR,
    [ControllerActionInputCDownButton]  = ControllerButtonIconCD,
    [ControllerActionInputCLeftButton]  = ControllerButtonIconCL,

    [ControllerActionInputDUpButton]    = ControllerButtonIconDU,
    [ControllerActionInputDRightButton] = ControllerButtonIconDR,
    [ControllerActionInputDDownButton]  = ControllerButtonIconDD,
    [ControllerActionInputDLeftButton]  = ControllerButtonIconDL,

    [ControllerActionInputZTrig]        = ControllerButtonIconZ,
    [ControllerActionInputRTrig]        = ControllerButtonIconR,
    [ControllerActionInputLTrig]        = ControllerButtonIconL,
};

static uint8_t sControllerActionInputToDirectionIcon[] = {
    [ControllerActionInputCUpButton]    = ControllerDirectionIconC,
    [ControllerActionInputDUpButton]    = ControllerDirectionIconD,
    [ControllerActionInputJoystick]     = ControllerDirectionIconJ,
};

struct ControlActionDataRow {
    enum StringId nameId;
    enum StringId headerId;
    enum ControllerAction action;
};

struct ControlActionDataRow sControllerDataRows[] = {
    {VALVE_MOVE,                VALVE_MOVEMENT_TITLE,                    ControllerActionMove},
    {VALVE_LOOK,                StringIdNone,                            ControllerActionRotate},
    {VALVE_JUMP,                StringIdNone,                            ControllerActionJump},
    {VALVE_DUCK,                StringIdNone,                            ControllerActionDuck},

    {VALVE_PRIMARY_ATTACK,      VALVE_COMBAT_TITLE,                      ControllerActionOpenPortal0},
    {VALVE_SECONDARY_ATTACK,    StringIdNone,                            ControllerActionOpenPortal1},
    {VALVE_USE_ITEMS,           StringIdNone,                            ControllerActionUseItem},

    {VALVE_PAUSE_GAME,          VALVE_MISCELLANEOUS_TITLE,               ControllerActionPause},

    {VALVE_LOOK_STRAIGHT_AHEAD, VALVE_MISCELLANEOUS_KEYBOARD_KEYS_TITLE, ControllerActionLookForward},
    {VALVE_LOOK_STRAIGHT_BACK,  StringIdNone,                            ControllerActionLookBackward},
    {VALVE_OVERVIEW_ZOOMIN,     StringIdNone,                            ControllerActionZoom},
};

enum ControllerAction controlsRowAction(int row) {
    return sControllerDataRows[row].action;
}

int controlsGetActionSourceIcons(enum ControllerAction action, struct ActionSourceIcon* sourceIcons) {
    struct ControllerActionSource sources[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controllerActionSources(action, sources, MAX_SOURCES_PER_CONTROLLER_ACTION);

    uint8_t* iconMapping;
    struct ControllerIcon* icons;

    if (ACTION_IS_DIRECTION(action)) {
        iconMapping = sControllerActionInputToDirectionIcon;
        icons = sControllerDirectionIcons;
    } else {
        iconMapping = sControllerActionInputToButtonIcon;
        icons = sControllerButtonIcons;
    }

    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerActionSource* source = &sources[i];
        struct ActionSourceIcon* sourceIcon = &sourceIcons[i];

        sourceIcon->inputIcon = &icons[iconMapping[source->input]];
        sourceIcon->controllerIndexIcon = &sControllerIndexIcons[source->controllerIndex];
    }

    return sourceCount;
}

struct ControllerIcon* controlsInputIcon(enum ControllerActionInput input) {
    if (input == ControllerActionInputJoystick) {
        return &sControllerDirectionIcons[ControllerDirectionIconJ];
    }

    return &sControllerButtonIcons[sControllerActionInputToButtonIcon[input]];
}

int controlsActionSourceIconsWidth(struct ActionSourceIcon* sourceIcons, int sourceCount) {
    int result = 0;

    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon* inputIcon = sourceIcons[i].inputIcon;
        result += inputIcon->w;
    }

    return result;
}

static void controlsMenuLayoutRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data, int x, int y) {
    struct PrerenderedText* copy = prerenderedTextCopy(row->actionText);
    menuFreePrerenderedDeferred(row->actionText);
    row->actionText = copy;
    prerenderedTextRelocate(row->actionText, x + ROW_PADDING_X, y);

    struct ActionSourceIcon sourceIcons[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controlsGetActionSourceIcons(data->action, sourceIcons);

    x = CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING_X;

    // Disambiguate between multiple bound controllers if necessary
    controlsRowBuildIcons(
        row,
        sourceIcons,
        sourceCount,
        x, y,
        controllerActionUsedControllerCount() > 1
    );

    row->y = y;
}

void controlsMenuLayoutHeader(struct ControlsMenuHeader* header, int x, int y) {
    struct PrerenderedText* copy = prerenderedTextCopy(header->headerText);
    menuFreePrerenderedDeferred(header->headerText);
    header->headerText = copy;
    prerenderedTextRelocate(header->headerText, x, y);
}

static void controlsMenuLayout(struct ControlsMenu* controlsMenu) {
    int y = CONTROLS_Y + controlsMenu->scrollOffset;
    int currentHeader = 0;

    int separatorCount = 0;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (sControllerDataRows[i].headerId != StringIdNone && currentHeader < MAX_CONTROLS_SECTIONS) {
            y += HEADER_PADDING_Y;
            controlsMenuLayoutHeader(&controlsMenu->headers[currentHeader], CONTROLS_X + HEADER_PADDING_X, y);
            y += HEADER_HEIGHT;

            if ((y + SEPARATOR_THICKNESS) > CONTROLS_Y_INNER && y < (CONTROLS_Y_INNER + CONTROLS_HT_INNER)) {
                controlsSeparatorAdd(controlsMenu, separatorCount, y);
                ++separatorCount;
            }
            y += SEPARATOR_PADDING_Y;
            ++currentHeader;
        }

        controlsMenuLayoutRow(&controlsMenu->actionRows[i], &sControllerDataRows[i], CONTROLS_X, y);
        
        y += controlsMenu->actionRows[i].actionText->height + ROW_PADDING_Y;
    }

    controlsSeparatorsFinish(controlsMenu, separatorCount);
}

static void controlsMenuInitRow(struct ControlsMenuRow* row, struct ControlActionDataRow* data) {
    row->actionText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(data->nameId), 0, 0, ROW_TEXT_MAX_WIDTH);

    controlsRowRenderInit(row);
}

static void controlsMenuInitHeader(struct ControlsMenuHeader* header, int message) {
    header->headerText = menuBuildPrerenderedText(&gDejaVuSansFont, translationsGet(message), 0, 0, SCREEN_WD);
}

static void controlsMenuInitText(struct ControlsMenu* controlsMenu) {
    int currentHeader = 0;

    for (int i = 0; i < ControllerActionCount; ++i) {
        if (sControllerDataRows[i].headerId != StringIdNone && currentHeader < MAX_CONTROLS_SECTIONS) {
            controlsMenuInitHeader(&controlsMenu->headers[currentHeader], sControllerDataRows[i].headerId);
            ++currentHeader;
        }

        controlsMenuInitRow(&controlsMenu->actionRows[i], &sControllerDataRows[i]);
    }

    for (; currentHeader < MAX_CONTROLS_SECTIONS; ++currentHeader) {
        controlsMenu->headers[currentHeader].headerText = NULL;
    }
}

void controlsMenuInit(struct ControlsMenu* controlsMenu) {
    controlsMenuInitText(controlsMenu);

    controlsMenu->selectedRow = 0;
    controlsMenu->scrollOffset = 0;
    controlsMenu->waitingForAction = ControllerActionNone;

    controlsMenuLayout(controlsMenu);

    controlsMenu->useDefaults = menuBuildButton(
        &gDejaVuSansFont, 
        translationsGet(GAMEUI_USEDEFAULTS),
        USE_DEFAULTS_X, USE_DEFAULTS_Y,
        USE_DEFAULTS_HEIGHT,
        1
    );

    controlsMenu->scrollOutline = menuBuildOutline(
        CONTROLS_X,
        CONTROLS_Y,
        CONTROLS_WIDTH,
        CONTROLS_HEIGHT,
        1
    );
}

void controlsMenuRebuildText(struct ControlsMenu* controlsMenu) {
    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedTextFree(controlsMenu->actionRows[i].actionText);
    }

    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        prerenderedTextFree(controlsMenu->headers[i].headerText);
    }

    controlsMenuInitText(controlsMenu);
    controlsMenuLayout(controlsMenu);
    menuRebuildButtonText(
        &controlsMenu->useDefaults,
        &gDejaVuSansFont,
        translationsGet(GAMEUI_USEDEFAULTS),
        1
    );
}

enum InputCapture controlsMenuUpdate(struct ControlsMenu* controlsMenu) {
    if (controlsMenu->waitingForAction != ControllerActionNone) {
        struct ControllerActionSource source;

        if (controllerActionReadAnySource(&source)) {
            if (controllerActionSetSource(controlsMenu->waitingForAction, &source, MAX_SOURCES_PER_CONTROLLER_ACTION)) {
                controlsMenuLayout(controlsMenu);
                soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
            } else {
                soundPlayerPlay(SOUNDS_WPN_DENYSELECT, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
            }

            controlsMenu->waitingForAction = ControllerActionNone;
        }

        return InputCaptureGrab;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        controlsMenu->selectedRow = controlsMenu->selectedRow + 1;

        if (controlsMenu->selectedRow == ControllerActionCount + 1) {
            controlsMenu->selectedRow = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        controlsMenu->selectedRow = controlsMenu->selectedRow - 1;

        if (controlsMenu->selectedRow < 0) {
            controlsMenu->selectedRow = ControllerActionCount;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];
        int newScroll = controlsMenu->scrollOffset;
        int topY = selectedAction->y;
        int bottomY = topY + selectedAction->actionText->height + ROW_PADDING_Y + OUTLINE_THICKNESS;

        if (sControllerDataRows[controlsMenu->selectedRow].headerId != StringIdNone) {
            topY -= HEADER_PADDING_Y + HEADER_HEIGHT + SEPARATOR_PADDING_Y;
        } else {
            topY -= ROW_PADDING_Y + OUTLINE_THICKNESS;
        }

        if (topY < CONTROLS_Y) {
            newScroll += CONTROLS_Y - topY;
        } else if (bottomY > (CONTROLS_Y + CONTROLS_HEIGHT)) {
            newScroll += CONTROLS_Y + CONTROLS_HEIGHT - bottomY;
        }

        if (newScroll != controlsMenu->scrollOffset) {
            controlsMenu->scrollOffset = newScroll;
            controlsMenuLayout(controlsMenu);
        }
    }

    if (controllerGetButtonsDown(0, ControllerButtonB)) {
        return InputCaptureExit;
    }

    if (controllerGetButtonsDown(0, ControllerButtonA)) {
        if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
            controlsMenu->waitingForAction = sControllerDataRows[controlsMenu->selectedRow].action;
        } else if (controlsMenu->selectedRow == ControllerActionCount) {
            controllerActionSetDefaultSources();
            controlsMenuLayout(controlsMenu);
        }

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    return InputCapturePass;
}

