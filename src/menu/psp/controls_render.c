#include "menu/controls.h"

#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "system/controller.h"
#include "util/memory.h"

#include "codegen/assets/materials/ui.h"

static struct Coloru8 sButtonPromptTextColor = { 232, 206, 80, 255 };

// A row's icons are resolved from its action when it is drawn, so laying the
// menu out leaves nothing behind.
void controlsRowRenderInit(struct ControlsMenuRow* row) {
    (void)row;
}

void controlsRowBuildIcons(struct ControlsMenuRow* row, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y, int showControllerIndex) {
    (void)row; (void)sourceIcons; (void)sourceCount; (void)x; (void)y; (void)showControllerIndex;
}

void controlsSeparatorAdd(struct ControlsMenu* controlsMenu, int index, int y) {
    if (index < 0 || index >= MAX_CONTROLS_SECTIONS) {
        return;
    }

    controlsMenu->render.separatorY[index] = (short)y;
}

void controlsSeparatorsFinish(struct ControlsMenu* controlsMenu, int count) {
    controlsMenu->render.separatorCount = (short)count;
}

static void controlsDrawIcon(struct RenderState* renderState, struct ControllerIcon* icon, int x, int y, unsigned int color) {
    const struct PspMaterial* material = ui_material_list[BUTTON_ICONS_INDEX];

    if (!material->texture) {
        return;
    }

    pspRenderTextureRect(
        renderState,
        material,
        x, y, icon->w, icon->h,
        icon->x,
        icon->y,
        icon->x + icon->w,
        icon->y + icon->h,
        color
    );
}

static int controlsDrawSourceInputIcons(struct RenderState* renderState, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y, unsigned int color) {
    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon* inputIcon = sourceIcons[i].inputIcon;

        x -= inputIcon->w;
        controlsDrawIcon(renderState, inputIcon, x, y, color);
    }

    return x;
}

static void controlsDrawSourceControllerIndexIcons(struct RenderState* renderState, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y, unsigned int color) {
    for (int i = 0; i < sourceCount; ++i) {
        struct ActionSourceIcon* sourceIcon = &sourceIcons[i];

        controlsDrawIcon(
            renderState,
            sourceIcon->controllerIndexIcon,
            x - sourceIcon->controllerIndexIcon->w,
            y + sourceIcon->inputIcon->h - sourceIcon->controllerIndexIcon->h,
            color
        );

        x -= sourceIcon->inputIcon->w;
    }
}

static void controlsDrawRowIcons(struct RenderState* renderState, struct ControlsMenu* controlsMenu, int row, unsigned int color) {
    struct ActionSourceIcon sourceIcons[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controlsGetActionSourceIcons(controlsRowAction(row), sourceIcons);

    int x = CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING_X;
    int y = controlsMenu->actionRows[row].y;

    controlsDrawSourceInputIcons(renderState, sourceIcons, sourceCount, x, y, color);

    if (controllerActionUsedControllerCount() > 1) {
        controlsDrawSourceControllerIndexIcons(renderState, sourceIcons, sourceCount, x, y, color);
    }
}

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    (void)task;

    struct Coloru8 panel = {0, 0, 0, 128};

    pspRenderFillRect(renderState, CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT, pspRenderColor(&panel));

    menuOutlineDraw(renderState, CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);

    for (int i = 0; i < controlsMenu->render.separatorCount; ++i) {
        pspRenderFillRect(
            renderState,
            CONTROLS_X + SEPARATOR_PADDING_X,
            controlsMenu->render.separatorY[i],
            CONTROLS_WIDTH - SEPARATOR_PADDING_X * 2,
            SEPARATOR_THICKNESS,
            pspRenderColor(&gColorBlack)
        );
    }

    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];
        struct Coloru8* highlight = controlsMenu->waitingForAction != ControllerActionNone
            ? &gSelectionGray
            : &gSelectionOrange;

        pspRenderFillRect(
            renderState,
            CONTROLS_X + ROW_PADDING_X,
            selectedAction->y,
            CONTROLS_WIDTH - ROW_PADDING_X * 2,
            controlsRowHeight(selectedAction),
            pspRenderColor(highlight)
        );
    }

    if (controlsMenu->selectedRow == ControllerActionCount) {
        pspRenderFillRect(
            renderState,
            controlsMenu->useDefaults.x,
            USE_DEFAULTS_Y,
            controlsMenu->useDefaults.w,
            USE_DEFAULTS_HEIGHT,
            pspRenderColor(&gSelectionOrange)
        );
    }

    menuButtonDraw(renderState, &controlsMenu->useDefaults);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    prerenderedBatchAdd(batch, controlsMenu->useDefaults.text,
        controlsMenu->selectedRow == ControllerActionCount ? &gColorBlack : &gColorWhite);
    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);

    pspRenderSetScissor(CONTROLS_X_INNER, CONTROLS_Y_INNER, CONTROLS_WD_INNER, CONTROLS_HT_INNER);

    batch = prerenderedBatchStart();

    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedBatchAdd(batch, controlsMenu->actionRows[i].actionText,
            controlsMenu->selectedRow == i ? &gColorBlack : &gColorWhite);
    }

    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        if (controlsMenu->headers[i].headerText == NULL) {
            break;
        }

        prerenderedBatchAdd(batch, controlsMenu->headers[i].headerText, &gColorWhite);
    }

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);

    for (int i = 0; i < ControllerActionCount; ++i) {
        controlsDrawRowIcons(renderState, controlsMenu, i,
            pspRenderColor(controlsMenu->selectedRow == i ? &gColorBlack : &gColorWhite));
    }

    pspRenderSetScissor(0, 0, SCREEN_WD, SCREEN_HT);
}

void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState) {
    if (message == NULL || *message == '\0') {
        return;
    }

    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (PROMPT_MARGIN_X + (PROMPT_PADDING * 2)));

    struct ActionSourceIcon sourceIcons[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controlsGetActionSourceIcons(action, sourceIcons);

    int iconsWidth = controlsActionSourceIconsWidth(sourceIcons, sourceCount);
    int opacityAsInt = (int)(255.0f * opacity);

    int textPositionX = (SCREEN_WD - PROMPT_MARGIN_X - PROMPT_PADDING) - fontRender->width;
    int textPositionY = (SCREEN_HT - PROMPT_MARGIN_Y - PROMPT_PADDING) - fontRender->height;

    struct Coloru8 back = {0, 0, 0, (unsigned char)(opacityAsInt / 3)};

    pspRenderFillRect(
        renderState,
        textPositionX - iconsWidth - (PROMPT_PADDING * 2),
        textPositionY - PROMPT_PADDING,
        (SCREEN_WD - PROMPT_MARGIN_X) - (textPositionX - iconsWidth - (PROMPT_PADDING * 2)),
        (SCREEN_HT - PROMPT_MARGIN_Y) - (textPositionY - PROMPT_PADDING),
        pspRenderColor(&back)
    );

    struct Coloru8 textColor = sButtonPromptTextColor;
    textColor.a = (unsigned char)opacityAsInt;

    fontRendererDraw(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState);

    controlsDrawSourceInputIcons(
        renderState, sourceIcons, sourceCount,
        textPositionX - PROMPT_PADDING, textPositionY,
        pspRenderColor(&textColor)
    );

    if (controllerActionUsedControllerCount() > 1) {
        struct Coloru8 indexColor = gColorWhite;
        indexColor.a = textColor.a;

        controlsDrawSourceControllerIndexIcons(
            renderState, sourceIcons, sourceCount,
            textPositionX - PROMPT_PADDING, textPositionY,
            pspRenderColor(&indexColor)
        );
    }

    stackMallocFree(fontRender);
}

void controlsRenderInputIcon(enum ControllerActionInput input, int x, int y, struct RenderState* renderState) {
    controlsDrawIcon(renderState, controlsInputIcon(input), x, y, pspRenderColor(&gColorWhite));
}
