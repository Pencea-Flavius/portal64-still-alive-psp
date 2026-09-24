#ifndef __SCENE_PSP_PEDESTAL_RENDER_H__
#define __SCENE_PSP_PEDESTAL_RENDER_H__

// The PSP half of the pedestal's drawing; see src/scene/n64/pedestal_render.h.
//
// The pedestal carries the portal gun as an armature attachment while it is
// up, which is the part the two machines disagree about.

#include "graphics/renderstate.h"
#include "scene/dynamic_render_list.h"

void pedestalRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
