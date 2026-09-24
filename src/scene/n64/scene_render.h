#ifndef __SCENE_N64_SCENE_RENDER_H__
#define __SCENE_N64_SCENE_RENDER_H__

// The N64 half of the scene's drawing; see src/scene/psp/scene_render.h.

#include "graphics/renderstate.h"

struct Scene;

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task);

#endif
