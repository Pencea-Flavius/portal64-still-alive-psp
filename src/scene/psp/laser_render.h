#ifndef __SCENE_PSP_LASER_RENDER_H__
#define __SCENE_PSP_LASER_RENDER_H__

// The PSP half of the laser's drawing; see src/scene/n64/laser_render.h.

#include "graphics/render_scene.h"
#include "math/transform.h"

void laserRender(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#endif
