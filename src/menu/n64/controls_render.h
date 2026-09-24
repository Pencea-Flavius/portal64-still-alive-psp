#ifndef __MENU_N64_CONTROLS_RENDER_H__
#define __MENU_N64_CONTROLS_RENDER_H__

// The N64 half of the controls menu's drawing; see src/menu/psp/controls_render.h.
// Rows and headers are built into display lists at layout time.

#include <ultra64.h>

#include "controls/controller_actions.h"
#include "graphics/renderstate.h"
#include "menu/menu.h"

#ifndef MAX_CONTROLS_SECTIONS
#error "include menu/controls.h rather than this directly"
#endif

#define SOURCE_ICON_COUNT ((MAX_SOURCES_PER_CONTROLLER_ACTION * GFX_ENTRIES_PER_IMAGE) + GFX_ENTRIES_PER_END_DL)

struct ControlsMenu;
struct ControlsMenuRow;
struct ActionSourceIcon;
struct ControllerIcon;

struct ControlsMenuRowRender {
    Gfx sourceInputIcons[SOURCE_ICON_COUNT];
    Gfx sourceControllerIndexIcons[SOURCE_ICON_COUNT];
};

struct ControlsMenuRender {
    Gfx headerSeparators[MAX_CONTROLS_SECTIONS + GFX_ENTRIES_PER_END_DL];
};

// Called by the shared layout for each row.
void controlsRowRenderInit(struct ControlsMenuRow* row);
void controlsRowBuildIcons(struct ControlsMenuRow* row, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y, int showControllerIndex);
void controlsSeparatorAdd(struct ControlsMenu* controlsMenu, int index, int y);
void controlsSeparatorsFinish(struct ControlsMenu* controlsMenu, int count);

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task);
void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState);
void controlsRenderInputIcon(enum ControllerActionInput input, int x, int y, struct RenderState* renderState);

#endif
