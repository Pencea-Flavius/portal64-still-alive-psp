#ifndef __SCENE_N64_TURRET_RENDER_H__
#define __SCENE_N64_TURRET_RENDER_H__

// The N64 half of the turret's drawing; see src/scene/psp/turret_render.h.
// The eye is an attachment with its own fade; see the N64 source for why.

#include "graphics/renderstate.h"
#include "scene/dynamic_render_list.h"

void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
