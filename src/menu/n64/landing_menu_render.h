#ifndef __MENU_N64_LANDING_MENU_RENDER_H__
#define __MENU_N64_LANDING_MENU_RENDER_H__

// The N64 half of the landing menu's drawing; see src/menu/psp/landing_menu_render.h.

#include "graphics/renderstate.h"

struct LandingMenu;

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif
