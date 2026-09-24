#ifndef __SCENE_N64_CAMERA_RENDER_H__
#define __SCENE_N64_CAMERA_RENDER_H__

// The N64 half of handing the camera to the renderer; see src/scene/psp/camera_render.h.
// Writes camera.c's float matrices in the RSP's fixed point layout.

#include <ultra64.h>

#include "graphics/render_types.h"
#include "graphics/renderstate.h"
#include "scene/camera.h"

int cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, RenderViewport viewport, int extractClippingPlanes, struct CameraMatrixInfo* output);
void cameraModifyProjectionViewForPortalGun(struct Camera* camera, struct RenderState* renderState, float newNearPlane, float aspectRatio);
int cameraApplyMatrices(struct RenderState* renderState, struct CameraMatrixInfo* matrixInfo);

#endif
