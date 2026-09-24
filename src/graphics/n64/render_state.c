#include "render_state.h"
#include "math/transform.h"

#include "initgfx.h"

#include "math/matrix.h"

void renderStateInit(struct RenderState* renderState, u16* framebuffer, u16* depthBuffer) {
    renderState->dl = renderState->glist;
    renderState->currentMemoryChunk = &renderState->glist[MAX_DL_LENGTH + MAX_RENDER_STATE_MEMORY_CHUNKS];
    renderState->framebuffer = framebuffer;
    renderState->depthBuffer = depthBuffer;
}

void* renderStateRequestMemory(struct RenderState* renderState, unsigned size) {
    unsigned memorySlots = (size + 7) >> 3;

    Gfx* result = renderState->currentMemoryChunk - memorySlots;

    // display list grows up, allocated memory grows down
    if (result <= renderState->dl) {
        return 0;
    }

    renderState->currentMemoryChunk = result;

    return result;
}

Mtx* renderStateRequestMatrices(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Mtx) * count);
}

Light* renderStateRequestLights(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Light) * count);
}

Vp* renderStateRequestViewport(struct RenderState* renderState) {
    return renderStateRequestMemory(renderState, sizeof(Vp));
}

Vtx* renderStateRequestVertices(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Vtx) * count);
}

LookAt* renderStateRequestLookAt(struct RenderState* renderState) {
    return renderStateRequestMemory(renderState, sizeof(LookAt));
}

void renderStateFlushCache(struct RenderState* renderState) {
    osWritebackDCache(renderState, sizeof(struct RenderState));
}

Gfx* renderStateAllocateDLChunk(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMemory(renderState, sizeof(Gfx) * count);
}

Gfx* renderStateReplaceDL(struct RenderState* renderState, Gfx* nextDL) {
    Gfx* result = renderState->dl;
    renderState->dl = nextDL;
    return result;
}

Gfx* renderStateStartChunk(struct RenderState* renderState) {
    return renderState->dl;    
}

Gfx* renderStateEndChunk(struct RenderState* renderState, Gfx* chunkStart) {
    Gfx* newChunk = renderStateAllocateDLChunk(renderState, (renderState->dl - chunkStart) + 1);
    Gfx* copyDest = newChunk;
    Gfx* copySrc = chunkStart;

    while (copySrc < renderState->dl) {
        *copyDest = *copySrc;
        ++copyDest;
        ++copySrc;
    }

    gSPEndDisplayList(copyDest++);

    renderState->dl = chunkStart;

    return newChunk;
}

int renderStateMaxDLCount(struct RenderState* renderState) {
    return renderState->currentMemoryChunk - renderState->glist;
}

void renderStateAppendDL(struct RenderState* renderState, Gfx* dl) {
    // Useful for small display lists that can be overwritten during rendering
    // E.g., menu elements
    while (_SHIFTR(dl->words.w0, 24, 8) != G_ENDDL) {
        *renderState->dl++ = *dl++;
    }
}

float renderStateMemoryUsage(struct RenderState* renderState) {
    int dlCount = renderState->dl - renderState->glist;
    int memoryChunkCount = &renderState->glist[MAX_DL_LENGTH + MAX_RENDER_STATE_MEMORY_CHUNKS] - renderState->currentMemoryChunk;

    return (float)(dlCount + memoryChunkCount) / (MAX_DL_LENGTH + MAX_RENDER_STATE_MEMORY_CHUNKS);
}

RenderMatrices renderStateTransformThroughPortal(struct RenderState* renderState, RenderMatrices transform, float portalTransform[4][4]) {
    Mtx* result = renderStateRequestMatrices(renderState, 1);

    if (!result) {
        return NULL;
    }

    float transformAsFloat[4][4];
    float finalTransform[4][4];

    guMtxL2F(transformAsFloat, transform);
    matrixMul(transformAsFloat, portalTransform, finalTransform);
    guMtxF2L(finalTransform, result);

    return result;
}

RenderViewport renderStateBuildViewport(struct RenderState* renderState, int minX, int minY, int maxX, int maxY, int minZ, int maxZ) {
    Vp* viewport = renderStateRequestViewport(renderState);

    if (!viewport) {
        return NULL;
    }

    viewport->vp.vscale[0] = (maxX - minX) << 1;
    viewport->vp.vscale[1] = (maxY - minY) << 1;
    viewport->vp.vscale[3] = 0;

    viewport->vp.vtrans[0] = (maxX + minX) << 1;
    viewport->vp.vtrans[1] = (maxY + minY) << 1;
    viewport->vp.vtrans[3] = 0;

    renderViewportSetDepthRange(viewport, minZ, maxZ);

    return viewport;
}

void renderViewportSetDepthRange(RenderViewport viewport, int minZ, int maxZ) {
    if (!viewport) {
        return;
    }

    viewport->vp.vscale[2] = (maxZ - minZ) >> 1;
    viewport->vp.vtrans[2] = (maxZ + minZ) >> 1;
}

RenderViewport renderViewportFullscreen() {
    return &fullscreenViewport;
}

RenderMatrices renderMatricesAt(RenderMatrices matrices, int index) {
    return &((Mtx*)matrices)[index];
}

RenderMatrices renderStateMatrixFromFloat(struct RenderState* renderState, float matrix[4][4]) {
    Mtx* result = renderStateRequestMatrices(renderState, 1);

    if (!result) {
        return NULL;
    }

    guMtxF2L(matrix, result);

    return result;
}

RenderMatrices renderStateTransformToMatrices(struct RenderState* renderState, struct Transform* transform, float sceneScale) {
    Mtx* result = renderStateRequestMatrices(renderState, 1);

    if (!result) {
        return NULL;
    }

    transformToMatrixL(transform, result, sceneScale);

    return result;
}

void renderMatrixFromTransform(RenderMatrix* into, struct Transform* transform, float sceneScale) {
    transformToMatrixL(transform, into, sceneScale);
}

void renderMatrixIdentity(RenderMatrix* into) {
    guMtxIdent(into);
}

RenderMatrices renderStateRequestMatrixBlock(struct RenderState* renderState, unsigned count) {
    return renderStateRequestMatrices(renderState, count);
}

void renderMatrixFlush(RenderMatrix* matrix) {
    osWritebackDCache(matrix, sizeof(Mtx));
}
