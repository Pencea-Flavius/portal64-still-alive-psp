#ifndef __MENU_PSP_CONTROLS_RENDER_H__
#define __MENU_PSP_CONTROLS_RENDER_H__

// The PSP half of the controls menu's drawing; see src/menu/n64/controls_render.h.
// Icons are drawn from the sources each frame; nothing is retained.

#include "controls/controller_actions.h"
#include "graphics/renderstate.h"
#include "menu/menu.h"

#ifndef MAX_CONTROLS_SECTIONS
#error "include menu/controls.h rather than this directly"
#endif

struct ControlsMenu;
struct ControlsMenuRow;
struct ActionSourceIcon;
struct ControllerIcon;

// Icons are resolved each frame. C has no empty struct, hence the field.
struct ControlsMenuRowRender {
    char nothingRetained;
};

// The separators' positions, which only the layout knows.
struct ControlsMenuRender {
    short separatorY[MAX_CONTROLS_SECTIONS];
    short separatorCount;
};

void controlsRowRenderInit(struct ControlsMenuRow* row);
void controlsRowBuildIcons(struct ControlsMenuRow* row, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y, int showControllerIndex);
void controlsSeparatorAdd(struct ControlsMenu* controlsMenu, int index, int y);
void controlsSeparatorsFinish(struct ControlsMenu* controlsMenu, int count);

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task);
void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState);
void controlsRenderInputIcon(enum ControllerActionInput input, int x, int y, struct RenderState* renderState);

#endif
