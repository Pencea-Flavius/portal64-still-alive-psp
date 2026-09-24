#ifndef __SCENE_N64_DOORWAY_COVER_RENDER_H__
#define __SCENE_N64_DOORWAY_COVER_RENDER_H__

// The N64 half of the doorway cover's drawing; see src/scene/psp/doorway_cover_render.h.

#include "graphics/render_scene.h"
#include "math/transform.h"

void doorwayCoverRender(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#endif
