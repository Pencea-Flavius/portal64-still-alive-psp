#include "camera_render.h"

#include "math/matrix.h"
#include "system/display.h"
#include "graphics/renderstate.h"
#include "graphics/psp/psp_model_render.h"

#include <pspgu.h>
#include <pspgum.h>

// The GU takes the engine's float matrices as they are: a copy.
static void cameraStoreMatrix(ScePspFMatrix4* out, float matrix[4][4]) {
    float* dst = (float*)out;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            dst[i * 4 + j] = matrix[i][j];
        }
    }
}

int cameraSetupMatrices(struct Camera* camera, struct RenderState* renderState, float aspectRatio, RenderViewport viewport, int extractClippingPlanes, struct CameraMatrixInfo* output) {
    // The viewport stays the whole screen and the scissor crops; it is only
    // folded into the culling, below.

    float view[4][4];
    float projection[4][4];
    float combined[4][4];

    cameraBuildProjectionMatrix(camera, projection, &output->perspectiveNormalize, aspectRatio);
    cameraBuildViewMatrix(camera, view);
    matrixMul(view, projection, combined);

    if (!cameraIsValidMatrix(combined)) {
        return 0;
    }

    // The combined matrix for the game, then view and projection apart for the
    // GE, whose fog needs the view space depth.
    ScePspFMatrix4* stored = renderStateRequestMemory(renderState, sizeof(ScePspFMatrix4) * 3);

    if (!stored) {
        return 0;
    }

    cameraStoreMatrix(&stored[0], combined);
    cameraStoreMatrix(&stored[1], view);
    cameraStoreMatrix(&stored[2], projection);
    output->projectionView = stored;

    // The N64's crop, for the culling planes only, so a portal view only draws
    // what its rectangle shows. Remaps the rectangle's x and y to -1..1.
    float culling[4][4];
    const struct RenderViewportRect* rect = viewport;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            culling[i][j] = combined[i][j];
        }
    }

    if (rect && rect->maxX > rect->minX && rect->maxY > rect->minY) {
        float scaleX = (float)(rect->maxX - rect->minX) / SCREEN_WD;
        float scaleY = (float)(rect->maxY - rect->minY) / SCREEN_HT;
        float centerX = (float)(rect->minX + rect->maxX - SCREEN_WD) / SCREEN_WD;
        float centerY = (float)(SCREEN_HT - rect->minY - rect->maxY) / SCREEN_HT;

        for (int i = 0; i < 4; ++i) {
            culling[i][0] = (combined[i][0] - centerX * combined[i][3]) / scaleX;
            culling[i][1] = (combined[i][1] - centerY * combined[i][3]) / scaleY;
        }
    }

    if (extractClippingPlanes) {
        cameraExtractClippingPlane(culling, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_RIGHT],  0,  1.0f);
        cameraExtractClippingPlane(culling, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_LEFT],   0, -1.0f);
        cameraExtractClippingPlane(culling, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_TOP],    1,  1.0f);
        cameraExtractClippingPlane(culling, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_BOTTOM], 1, -1.0f);
        cameraExtractClippingPlane(culling, &output->cullingInformation.clippingPlanes[CLIPPING_PLANE_NEAR],   2,  1.0f);
        output->cullingInformation.cameraPos = camera->transform.position;
        output->cullingInformation.usedClippingPlaneCount = 5;
    }

    return 1;
}

void cameraModifyProjectionViewForPortalGun(struct Camera* camera, struct RenderState* renderState, float newNearPlane, float aspectRatio) {
    (void)renderState;

    struct Camera portalCam = *camera;
    portalCam.nearPlane = newNearPlane;
    portalCam.transform.position = gZeroVec;

    float view[4][4];
    float projectionView[4][4];
    unsigned short perspectiveNormalize;

    cameraBuildProjectionMatrix(&portalCam, projectionView, &perspectiveNormalize, aspectRatio);
    cameraBuildViewMatrix(&portalCam, view);
    matrixMul(view, projectionView, projectionView);

    ScePspFMatrix4 combined;
    cameraStoreMatrix(&combined, projectionView);

    // No gSPPerspNormalize: the GE works in floats.
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadMatrix(&combined);
    // The view is inside the combined matrix here.
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    pspModelViewChanged();
}

int cameraApplyMatrices(struct RenderState* renderState, struct CameraMatrixInfo* matrixInfo) {
    (void)renderState;

    if (!matrixInfo->projectionView) {
        return 0;
    }

    // Projection and view loaded apart (the GE's fog needs view depth); the
    // model stack starts at identity.
    const ScePspFMatrix4* matrices = (const ScePspFMatrix4*)matrixInfo->projectionView;

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadMatrix(&matrices[2]);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadMatrix(&matrices[1]);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    pspModelViewChanged();

    return 1;
}
