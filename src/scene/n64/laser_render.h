#ifndef __SCENE_N64_LASER_RENDER_H__
#define __SCENE_N64_LASER_RENDER_H__

// The N64 half of the laser's drawing; see src/scene/psp/laser_render.h.

#include "graphics/render_scene.h"
#include "math/transform.h"

void laserRender(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#endif
