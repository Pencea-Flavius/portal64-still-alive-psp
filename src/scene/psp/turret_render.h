#ifndef __SCENE_PSP_TURRET_RENDER_H__
#define __SCENE_PSP_TURRET_RENDER_H__

// The PSP half of the turret's drawing; see src/scene/n64/turret_render.h.
// The eye is drawn with its own fade over a fizzlable body; see the source.

#include "graphics/renderstate.h"
#include "scene/dynamic_render_list.h"

void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
