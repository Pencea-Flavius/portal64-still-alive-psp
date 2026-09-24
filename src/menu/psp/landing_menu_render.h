#ifndef __MENU_PSP_LANDING_MENU_RENDER_H__
#define __MENU_PSP_LANDING_MENU_RENDER_H__

// The PSP half of the landing menu's drawing; see src/menu/n64/landing_menu_render.h.

#include "graphics/renderstate.h"

struct LandingMenu;

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif
