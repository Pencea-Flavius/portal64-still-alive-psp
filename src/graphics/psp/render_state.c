#include "render_state.h"

#include "graphics/psp/psp_model_render.h"

#include "math/matrix.h"
#include "math/transform.h"
#include "system/display.h"

#include <pspgu.h>
#include <string.h>
#include <pspkernel.h>
#include <pspgum.h>

void renderStateReset(struct RenderState* renderState) {
    renderState->used = 0;
}

void renderStateInit(struct RenderState* renderState) {
    renderState->used = 0;
    // Drop cached copies so nothing dirty overwrites the uncached writes.
    sceKernelDcacheWritebackInvalidateRange(renderState->scratch, RENDER_STATE_SCRATCH_SIZE);
}

void* renderStateRequestMemory(struct RenderState* renderState, unsigned size) {
    // The GE wants 16 byte boundaries, so every allocation starts on one.
    unsigned aligned = (renderState->used + 15) & ~15u;

    if (aligned + size > RENDER_STATE_SCRATCH_SIZE) {
        return 0;
    }

    renderState->used = aligned + size;

    // Uncached: the GE draws while the CPU is still writing the frame, so a
    // flush at the end is too late (PPSSPP has no cache and hid this).
    return (void*)((unsigned int)&renderState->scratch[aligned] | 0x40000000);
}

float renderStateMemoryUsage(const struct RenderState* renderState) {
    return (float)renderState->used / (float)RENDER_STATE_SCRATCH_SIZE;
}

// An object's clone on the far side of a portal it passes through: its
// transform times the portal's, as on the N64.
RenderMatrices renderStateTransformThroughPortal(struct RenderState* renderState, RenderMatrices transform, float portalTransform[4][4]) {
    ScePspFMatrix4* result = renderStateRequestMemory(renderState, sizeof(ScePspFMatrix4));

    if (!result || !transform) {
        return NULL;
    }

    float transformAsFloat[4][4];
    float finalTransform[4][4];

    memcpy(transformAsFloat, transform, sizeof(transformAsFloat));
    matrixMul(transformAsFloat, portalTransform, finalTransform);
    memcpy(result, finalTransform, sizeof(finalTransform));

    return result;
}

RenderViewport renderStateBuildViewport(struct RenderState* renderState, int minX, int minY, int maxX, int maxY, int minZ, int maxZ) {
    struct RenderViewportRect* viewport = renderStateRequestMemory(renderState, sizeof(struct RenderViewportRect));

    if (!viewport) {
        return NULL;
    }

    viewport->minX = (short)minX;
    viewport->minY = (short)minY;
    viewport->maxX = (short)maxX;
    viewport->maxY = (short)maxY;

    renderViewportSetDepthRange(viewport, minZ, maxZ);

    return viewport;
}

void renderViewportSetDepthRange(RenderViewport handle, int minZ, int maxZ) {
    struct RenderViewportRect* viewport = handle;

    if (!viewport) {
        return;
    }

    viewport->minZ = (unsigned short)minZ;
    viewport->maxZ = (unsigned short)maxZ;
}

// Static: it never changes and is needed before the first frame.
static struct RenderViewportRect sFullscreenViewport = {
    0, 0, SCREEN_WD, SCREEN_HT, 0, RENDER_MAX_DEPTH
};

RenderViewport renderViewportFullscreen() {
    return &sFullscreenViewport;
}

void renderViewportApply(RenderViewport handle) {
    struct RenderViewportRect* viewport = handle;

    if (!viewport) {
        return;
    }

    // Always the whole screen; the stage's scissor (renderStageApply()) crops,
    // as the N64's cropped projection does.
    sceGuOffset(2048 - (SCREEN_WD / 2), 2048 - (SCREEN_HT / 2));
    sceGuViewport(2048, 2048, SCREEN_WD, SCREEN_HT);

    // The plan's slices are N64 style (small is near); the GE's depth is
    // reversed, so mirror them, and stop short of the top to leave room for
    // depth offsets (see pspWidenDepthWindow()).
    int top = RENDER_MAX_DEPTH - PSP_DEPTH_HEADROOM;
    int nearZ = top - (int)(((long long)viewport->minZ * top) / RENDER_MAX_DEPTH);
    int farZ = top - (int)(((long long)viewport->maxZ * top) / RENDER_MAX_DEPTH);

    sceGuDepthRange(nearZ, farZ);
    pspDepthSetWindow(nearZ, farZ);
    pspWidenDepthWindow();
}

RenderMatrices renderMatricesAt(RenderMatrices matrices, int index) {
    return &((ScePspFMatrix4*)matrices)[index];
}

RenderMatrices renderStateMatrixFromFloat(struct RenderState* renderState, float matrix[4][4]) {
    ScePspFMatrix4* result = renderStateRequestMemory(renderState, sizeof(ScePspFMatrix4));

    if (!result) {
        return NULL;
    }

    float* stored = (float*)result;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            stored[i * 4 + j] = matrix[i][j];
        }
    }

    return result;
}

RenderMatrices renderStateTransformToMatrices(struct RenderState* renderState, struct Transform* transform, float sceneScale) {
    float asFloats[4][4];

    transformToMatrix(transform, asFloats, sceneScale);

    return renderStateMatrixFromFloat(renderState, asFloats);
}

void renderMatrixFromTransform(RenderMatrix* into, struct Transform* transform, float sceneScale) {
    float asFloats[4][4];

    transformToMatrix(transform, asFloats, sceneScale);

    float* stored = (float*)into;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            stored[i * 4 + j] = asFloats[i][j];
        }
    }
}

void renderMatrixIdentity(RenderMatrix* into) {
    gumLoadIdentity(into);
}

RenderMatrices renderStateRequestMatrixBlock(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(ScePspFMatrix4) * count);
}

void renderMatrixFlush(RenderMatrix* matrix) {
    sceKernelDcacheWritebackRange(matrix, sizeof(ScePspFMatrix4));
}
