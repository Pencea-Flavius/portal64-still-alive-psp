#ifndef __SCENE_PSP_SCENE_RENDER_H__
#define __SCENE_PSP_SCENE_RENDER_H__

// The PSP half of the scene's drawing; see src/scene/n64/scene_render.h.

#include "graphics/renderstate.h"

struct Scene;

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task);

#endif
