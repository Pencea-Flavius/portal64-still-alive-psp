#ifndef __SCENE_PSP_DOORWAY_COVER_RENDER_H__
#define __SCENE_PSP_DOORWAY_COVER_RENDER_H__

// The PSP half of the doorway cover's drawing; see src/scene/n64/doorway_cover_render.h.

#include "graphics/render_scene.h"
#include "math/transform.h"

void doorwayCoverRender(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#endif
