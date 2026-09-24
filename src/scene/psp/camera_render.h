#ifndef __SCENE_PSP_CAMERA_RENDER_H__
#define __SCENE_PSP_CAMERA_RENDER_H__

// The PSP half of handing the camera to the renderer; see src/scene/n64/camera_render.h.
// The viewport stays the whole screen and the stage's scissor crops (see
// renderViewportApply()), so the projection is the plain perspective.

#include "graphics/renderstate.h"
#include "scene/camera.h"

int cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, RenderViewport viewport, int extractClippingPlanes, struct CameraMatrixInfo* output);
void cameraModifyProjectionViewForPortalGun(struct Camera* camera, struct RenderState* renderState, float newNearPlane, float aspectRatio);
int cameraApplyMatrices(struct RenderState* renderState, struct CameraMatrixInfo* matrixInfo);

#endif
