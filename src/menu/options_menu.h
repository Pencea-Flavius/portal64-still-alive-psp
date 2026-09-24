#ifndef __MENU_OPTIONS_MENU_H__
#define __MENU_OPTIONS_MENU_H__

#include "audio_options.h"
#include "controls.h"
#include "gameplay_options.h"
#include "graphics/render_types.h"
#include "joystick_options.h"
#include "menu.h"
#include "tabs.h"
#include "video_options.h"
#include "system/display.h"

// The frame the tabs sit in. Both halves draw against it, so it lives here
// rather than in the source, and it is spelled out in full: four other menus
// have a MENU_WIDTH of their own.
// The box's size and place are in menu.h, where the pages it holds can see them.

#define OPTIONS_PADDING 8

enum OptionsMenuTabs {
    OptionsMenuTabsControlMapping,
    OptionsMenuTabsControlJoystick,
    OptionsMenuTabsAudio,
    OptionsMenuTabsVideo,
    OptionsMenuTabsGameplay,

    OptionsMenuTabsCount,
};

struct OptionsMenu {
    RenderDisplayList menuOutline;

    struct Tabs tabs;

    struct ControlsMenu controlsMenu;
    struct JoystickOptions joystickOptions;
    struct AudioOptions audioOptions;
    struct VideoOptions videoOptions;
    struct GameplayOptions gameplayOptions;
};

void optionsMenuInit(struct OptionsMenu* options);
void optionsMenuRebuildText(struct OptionsMenu* options);
enum InputCapture optionsMenuUpdate(struct OptionsMenu* options);

// Drawing is declared in the platform's half, which the build puts on the
// include path.
#include "options_menu_render.h"

#endif