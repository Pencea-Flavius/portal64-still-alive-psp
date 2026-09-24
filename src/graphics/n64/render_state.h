#ifndef __GRAPHICS_N64_RENDER_STATE_H__
#define __GRAPHICS_N64_RENDER_STATE_H__

// The N64 half of the render state; see src/graphics/psp/render_state.h.
// The frame's display list, with a bump allocator growing down from its far
// end for matrices, lights, viewports and vertices.

#include <ultra64.h>

#include "graphics/render_types.h"

#define MAX_DL_LENGTH           2000
#define MAX_RENDER_STATE_MEMORY 12800
#define MAX_RENDER_STATE_MEMORY_CHUNKS (MAX_RENDER_STATE_MEMORY / sizeof(u64))
#define MAX_DYNAMIC_LIGHTS      128

struct RenderState {
    Gfx glist[MAX_DL_LENGTH + MAX_RENDER_STATE_MEMORY_CHUNKS];
    Gfx* dl;
    u16* framebuffer;
    u16* depthBuffer;
    Gfx* currentMemoryChunk;
};

void renderStateInit(struct RenderState* renderState, u16* framebuffer, u16* depthBuffer);
// Returns NULL when the display list has grown into the memory chunks.
void* renderStateRequestMemory(struct RenderState* renderState, unsigned size);
Mtx* renderStateRequestMatrices(struct RenderState* renderState, unsigned count);
Light* renderStateRequestLights(struct RenderState* renderState, unsigned count);
Vp* renderStateRequestViewport(struct RenderState* renderState);
Vtx* renderStateRequestVertices(struct RenderState* renderState, unsigned count);
LookAt* renderStateRequestLookAt(struct RenderState* renderState);
void renderStateFlushCache(struct RenderState* renderState);
Gfx* renderStateAllocateDLChunk(struct RenderState* renderState, unsigned count);
Gfx* renderStateReplaceDL(struct RenderState* renderState, Gfx* nextDL);
Gfx* renderStateStartChunk(struct RenderState* renderState);
Gfx* renderStateEndChunk(struct RenderState* renderState, Gfx* chunkStart);

int renderStateMaxDLCount(struct RenderState* renderState);

void renderStateAppendDL(struct RenderState* renderState, Gfx* dl);

// A transform put through a portal, kept in the frame.
RenderMatrices renderStateTransformThroughPortal(struct RenderState* renderState, RenderMatrices transform, float portalTransform[4][4]);

// The nth matrix of a block the renderer was handed.
RenderMatrices renderMatricesAt(RenderMatrices matrices, int index);

// A float matrix, copied into the frame in the renderer's layout.
RenderMatrices renderStateMatrixFromFloat(struct RenderState* renderState, float matrix[4][4]);

// A transform, put into the frame as a matrix. NULL when the frame is full.
struct Transform;
RenderMatrices renderStateTransformToMatrices(struct RenderState* renderState, struct Transform* transform, float sceneScale);

// A block of matrices in the frame, filled by the caller with
// renderMatrixFromTransform(). NULL when the frame is full.
RenderMatrices renderStateRequestMatrixBlock(struct RenderState* renderState, unsigned count);

// A transform written into a matrix a game object owns.
void renderMatrixFromTransform(RenderMatrix* into, struct Transform* transform, float sceneScale);

// The same, set to identity.
void renderMatrixIdentity(RenderMatrix* into);

// Writes a game-owned matrix back to memory for DMA.
void renderMatrixFlush(RenderMatrix* matrix);

// Depth range the render plan slices between stages.
#define RENDER_MAX_DEPTH    G_MAXZ

// A stage's screen rectangle and depth slice, in the frame.
RenderViewport renderStateBuildViewport(struct RenderState* renderState, int minX, int minY, int maxX, int maxY, int minZ, int maxZ);

// Changes a viewport's depth slice.
void renderViewportSetDepthRange(RenderViewport viewport, int minZ, int maxZ);

// The whole screen, which the root stage's matrices are set up against.
RenderViewport renderViewportFullscreen();

float renderStateMemoryUsage(struct RenderState* renderState);

#endif