#ifndef __SCENE_N64_HUD_RENDER_H__
#define __SCENE_N64_HUD_RENDER_H__

// The N64 half of the HUD's drawing; see src/scene/psp/hud_render.h.

#include "graphics/renderstate.h"

struct Hud;
struct Player;

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState);

#endif
