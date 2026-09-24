#ifndef __SCENE_N64_PEDESTAL_RENDER_H__
#define __SCENE_N64_PEDESTAL_RENDER_H__

// The N64 half of the pedestal's drawing; see src/scene/psp/pedestal_render.h.
// The gun is an attachment while the pedestal is up.

#include "graphics/renderstate.h"
#include "scene/dynamic_render_list.h"

void pedestalRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
