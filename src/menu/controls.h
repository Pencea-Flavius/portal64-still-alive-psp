#ifndef __MENU_CONTROLS_H__
#define __MENU_CONTROLS_H__

#include "controls/controller_actions.h"
#include "font/font.h"
#include "graphics/render_types.h"
#include "menu.h"
#include "scene/hud.h"
#include "system/display.h"

#define MAX_CONTROLS_SECTIONS             4
#define MAX_SOURCES_PER_CONTROLLER_ACTION 4

#define CONTROLS_WIDTH      252
#define CONTROLS_HEIGHT     124
#define CONTROLS_X          ((SCREEN_WD - CONTROLS_WIDTH) / 2)
#define CONTROLS_Y          OPTIONS_PAGE_TOP

#define OUTLINE_THICKNESS   1
#define CONTROLS_WD_INNER   (CONTROLS_WIDTH  - OUTLINE_THICKNESS)
#define CONTROLS_HT_INNER   (CONTROLS_HEIGHT - OUTLINE_THICKNESS)
#define CONTROLS_X_INNER    (CONTROLS_X      + OUTLINE_THICKNESS)
#define CONTROLS_Y_INNER    (CONTROLS_Y      + OUTLINE_THICKNESS)

#define HEADER_PADDING_X    2
#define HEADER_PADDING_Y    4
#define HEADER_HEIGHT       14

#define SEPARATOR_PADDING_X 8
#define SEPARATOR_PADDING_Y 3
#define SEPARATOR_THICKNESS 1

#define ROW_PADDING_X       8
#define ROW_PADDING_Y       2
#define ROW_TEXT_MAX_WIDTH  190

// Offsets from the box's corner.
#define USE_DEFAULTS_X      (OPTIONS_MENU_LEFT + OPTIONS_MENU_WIDTH - 14)
#define USE_DEFAULTS_Y      (OPTIONS_MENU_TOP + 166)
#define USE_DEFAULTS_HEIGHT 16

#define PROMPT_MARGIN_X     17
#define PROMPT_MARGIN_Y     72
#define PROMPT_HEIGHT       24
#define PROMPT_PADDING      6

// Where an icon sits in the button atlas.
struct ControllerIcon {
    char x, y;
    char w, h;
};

// The action a row shows (not the row's index).
enum ControllerAction controlsRowAction(int row);

struct ActionSourceIcon {
    struct ControllerIcon* inputIcon;
    struct ControllerIcon* controllerIndexIcon;
};

// Drawing is the platform's half; included here because these structures
// hold its types.
#include "controls_render.h"

struct ControlsMenuHeader {
    struct PrerenderedText* headerText;
};

struct ControlsMenuRow {
    struct PrerenderedText* actionText;
    struct ControlsMenuRowRender render;
    short y;
};

struct ControlsMenu {
    RenderDisplayList scrollOutline;
    struct ControlsMenuRender render;

    struct ControlsMenuHeader headers[MAX_CONTROLS_SECTIONS];
    struct ControlsMenuRow actionRows[ControllerActionCount];
    struct MenuButton useDefaults;

    short selectedRow;
    short scrollOffset;
    enum ControllerAction waitingForAction;
};

void controlsMenuInit(struct ControlsMenu* controlsMenu);
void controlsMenuRebuildText(struct ControlsMenu* controlsMenu);
enum InputCapture controlsMenuUpdate(struct ControlsMenu* controlsMenu);

// An action's icons, for both halves of the drawing.
int controlsGetActionSourceIcons(enum ControllerAction action, struct ActionSourceIcon* sourceIcons);
int controlsActionSourceIconsWidth(struct ActionSourceIcon* sourceIcons, int sourceCount);
struct ControllerIcon* controlsInputIcon(enum ControllerActionInput input);

#endif
