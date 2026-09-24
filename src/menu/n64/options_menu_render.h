#ifndef __MENU_N64_OPTIONS_MENU_RENDER_H__
#define __MENU_N64_OPTIONS_MENU_RENDER_H__

// The N64 half of the options menu's drawing; see src/menu/psp/options_menu_render.h.

#include "graphics/renderstate.h"

struct OptionsMenu;

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task);

#endif
