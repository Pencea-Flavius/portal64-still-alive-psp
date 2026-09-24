#include "landing_menu.h"

#include "audio/soundplayer.h"
#include "cheat_codes.h"
#include "font/font.h"
#include "font/dejavu_sans.h"
#include "strings/translations.h"
#include "system/controller.h"
#include "system/display.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"

#include "codegen/version.h"

#define LANDING_MENU_TEXT_START_X   30
#define LANDING_MENU_TEXT_START_Y   132
#define STRIDE_OPTION_1             12
#define STRIDE_OPTION_2             16

#define LANDING_MENU_VERSION_END_Y  20

void landingMenuInitText(struct LandingMenu* landingMenu) {
    int y = LANDING_MENU_TEXT_START_Y;
    int stride = getCurrentStrideValue(landingMenu);

    for (int i = 0; i < landingMenu->optionCount; ++i) {
        landingMenu->optionText[i] = menuBuildPrerenderedText(&gDejaVuSansFont,
            translationsGet(landingMenu->options[i].messageId),
            LANDING_MENU_TEXT_START_X,
            y,
            SCREEN_WD);
        y += stride;
    }

    landingMenu->versionText = menuBuildPrerenderedText(&gDejaVuSansFont, GAME_VERSION, 0, 0, SCREEN_WD);
    prerenderedTextRelocate(
        landingMenu->versionText,
        SCREEN_WD - LANDING_MENU_TEXT_START_X - landingMenu->versionText->width,
        SCREEN_HT - LANDING_MENU_VERSION_END_Y - landingMenu->versionText->height
    );
}

void landingMenuInit(struct LandingMenu* landingMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground) {
    landingMenu->optionText = malloc(sizeof(struct PrerenderedText*) * optionCount);
    landingMenu->options = options;
    landingMenu->versionText = NULL;
    landingMenu->selectedItem = 0;
    landingMenu->optionCount = optionCount;
    landingMenu->darkenBackground = darkenBackground;
    landingMenuInitText(landingMenu);
}

void landingMenuRebuildText(struct LandingMenu* landingMenu) {
    for (int i = 0; i < landingMenu->optionCount; ++i) {
        prerenderedTextFree(landingMenu->optionText[i]);
    }
    prerenderedTextFree(landingMenu->versionText);

    landingMenuInitText(landingMenu);
}

struct LandingMenuOption* landingMenuUpdate(struct LandingMenu* landingMenu) {
    enum ControllerDirection dir = controllerGetDirectionDown(0);

    if (dir & ControllerDirectionUp) {
        if (landingMenu->selectedItem > 0) {
            --landingMenu->selectedItem;
        } else {
            landingMenu->selectedItem = landingMenu->optionCount - 1;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);

        cheatCodeEnterDirection(CheatCodeDirUp);
    }

    if (dir & ControllerDirectionDown) {
        if (landingMenu->selectedItem + 1 < landingMenu->optionCount) {
            ++landingMenu->selectedItem;
        } else {
            landingMenu->selectedItem = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);

        cheatCodeEnterDirection(CheatCodeDirDown);
    }

    if (dir & ControllerDirectionLeft) {
        cheatCodeEnterDirection(CheatCodeDirLeft);
    }

    if (dir & ControllerDirectionRight) {
        cheatCodeEnterDirection(CheatCodeDirRight);
    }

    if (controllerGetButtonsDown(0, ControllerButtonA)) {
        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
        return &landingMenu->options[landingMenu->selectedItem];
    }

    return NULL;
}

int getCurrentStrideValue(struct LandingMenu* landingMenu)
{
    return (landingMenu->optionCount > PACKED_MENU_THRESHOLD ? STRIDE_OPTION_1 : STRIDE_OPTION_2);
}
