#ifndef __MENU_PSP_OPTIONS_MENU_RENDER_H__
#define __MENU_PSP_OPTIONS_MENU_RENDER_H__

// The PSP half of the options menu's drawing; see src/menu/n64/options_menu_render.h.

#include "graphics/renderstate.h"

struct OptionsMenu;

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task);

#endif
