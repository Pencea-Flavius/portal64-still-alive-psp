#include "options_menu.h"
#include "audio/soundplayer.h"

#include "font/font.h"
#include "font/dejavu_sans.h"
#include "savefile/savefile.h"
#include "system/controller.h"
#include "system/display.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/strings/strings.h"


struct Tab gOptionTabs[] = {
    {
        .messageId = OPTIONS_CONTROLS,
    },
    {
        .messageId = GAMEUI_JOYSTICK_N64,
    },
    {
        .messageId = GAMEUI_AUDIO,
    },
    {
        .messageId = GAMEUI_VIDEO,
    },
    {
        .messageId = GAMEUI_PORTAL,
    },
};

void optionsMenuInit(struct OptionsMenu* options) {
    options->menuOutline = menuBuildBorder(OPTIONS_MENU_LEFT, OPTIONS_MENU_TOP, OPTIONS_MENU_WIDTH, OPTIONS_MENU_HEIGHT);

    tabsInit(
        &options->tabs, 
        gOptionTabs, 
        sizeof(gOptionTabs) / sizeof(*gOptionTabs), 
        &gDejaVuSansFont,
        OPTIONS_MENU_LEFT + OPTIONS_PADDING, OPTIONS_MENU_TOP + OPTIONS_PADDING,
        OPTIONS_MENU_WIDTH - OPTIONS_PADDING * 2, OPTIONS_MENU_HEIGHT - OPTIONS_PADDING * 2
    );

    controlsMenuInit(&options->controlsMenu);
    joystickOptionsInit(&options->joystickOptions);
    audioOptionsInit(&options->audioOptions);
    videoOptionsInit(&options->videoOptions);
    gameplayOptionsInit(&options->gameplayOptions);
}

void optionsMenuRebuildText(struct OptionsMenu* options) {
    controlsMenuRebuildText(&options->controlsMenu);
    joystickOptionsRebuildText(&options->joystickOptions);
    audioOptionsRebuildtext(&options->audioOptions);
    videoOptionsRebuildtext(&options->videoOptions);
    gameplayOptionsRebuildText(&options->gameplayOptions);
    tabsRebuildText(&options->tabs);
}

enum InputCapture optionsMenuUpdate(struct OptionsMenu* options) {
    enum InputCapture result = InputCapturePass;

    switch (options->tabs.selectedTab) {
        case OptionsMenuTabsControlMapping:
            result = controlsMenuUpdate(&options->controlsMenu);
            break;
        case OptionsMenuTabsControlJoystick:
            result = joystickOptionsUpdate(&options->joystickOptions);
            break;
        case OptionsMenuTabsAudio:
            result = audioOptionsUpdate(&options->audioOptions);
            break;
        case OptionsMenuTabsVideo:
            result = videoOptionsUpdate(&options->videoOptions);
            break;
        case OptionsMenuTabsGameplay:
            result = gameplayOptionsUpdate(&options->gameplayOptions);
            break;
    }

    if (result == InputCaptureExit) {
        savefileSave();
        return InputCaptureExit;
    } else if (result != InputCapturePass) {
        return result;
    }

    if (controllerGetButtonsDown(0, ControllerButtonZ | ControllerButtonL)) {
        if (options->tabs.selectedTab == 0) {
            tabsSetSelectedTab(&options->tabs, OptionsMenuTabsCount - 1);
        } else {
            tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab - 1);
        }

        tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab);
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    if (controllerGetButtonsDown(0, ControllerButtonR)) {
        if (options->tabs.selectedTab == OptionsMenuTabsCount - 1) {
            tabsSetSelectedTab(&options->tabs, 0);
        } else {
            tabsSetSelectedTab(&options->tabs, options->tabs.selectedTab + 1);
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);

    }

    return InputCapturePass;
}
