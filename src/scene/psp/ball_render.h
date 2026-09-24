#ifndef __SCENE_PSP_BALL_RENDER_H__
#define __SCENE_PSP_BALL_RENDER_H__

// The PSP half of the energy ball's drawing; see src/scene/n64/ball_render.h.

#include "graphics/render_scene.h"
#include "math/transform.h"
#include "scene/dynamic_render_list.h"

void ballRender(void* data, struct RenderScene* renderScene, struct Transform* fromView);
void ballBurnRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
