#ifndef __SCENE_PSP_HUD_RENDER_H__
#define __SCENE_PSP_HUD_RENDER_H__

// The PSP half of the HUD's drawing; see src/scene/n64/hud_render.h.

#include "graphics/renderstate.h"

struct Hud;
struct Player;

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState);

#endif
