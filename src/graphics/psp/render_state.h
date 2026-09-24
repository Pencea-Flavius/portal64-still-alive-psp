#ifndef __GRAPHICS_PSP_RENDER_STATE_H__
#define __GRAPHICS_PSP_RENDER_STATE_H__

// The PSP half of the render state; see src/graphics/n64/render_state.h.
// Matrices, lights and the list belong to sceGu; what is left is a scratch
// arena for data the GE reads asynchronously during the frame.

#include "graphics/render_types.h"

// Everything a frame hands the GE comes from here (stage matrices, sprites,
// frame geometry, material clones). 32KB ran out with portals open.
#define RENDER_STATE_SCRATCH_SIZE 0x40000

struct RenderState {
    // Whole cache lines, so `used` never shares one with the scratch, which is
    // only touched through its uncached alias.
    unsigned char scratch[RENDER_STATE_SCRATCH_SIZE] __attribute__((aligned(64)));
    unsigned int  used;
};

// Once, before the first frame.
void renderStateInit(struct RenderState* renderState);

// Once per frame, after the GE has synced.
void renderStateReset(struct RenderState* renderState);

// Returns NULL when exhausted; callers then draw nothing.
void* renderStateRequestMemory(struct RenderState* renderState, unsigned size);

float renderStateMemoryUsage(const struct RenderState* renderState);

// A model's transform through a portal. Not implemented on the PSP yet:
// returns NULL and the clone is not drawn.
RenderMatrices renderStateTransformThroughPortal(struct RenderState* renderState, RenderMatrices transform, float portalTransform[4][4]);

// The nth matrix of a block (e.g. bone transforms); the layout is per
// machine.
RenderMatrices renderMatricesAt(RenderMatrices matrices, int index);

// A float matrix copied into the frame in the renderer's layout.
RenderMatrices renderStateMatrixFromFloat(struct RenderState* renderState, float matrix[4][4]);

// A transform converted into a frame matrix, so render code needn't name a
// machine. NULL when the frame is full.
struct Transform;
RenderMatrices renderStateTransformToMatrices(struct RenderState* renderState, struct Transform* transform, float sceneScale);

// A block of frame matrices for code that poses things itself (elevator
// doors, fizzler sides); index with renderMatricesAt(). NULL when full.
RenderMatrices renderStateRequestMatrixBlock(struct RenderState* renderState, unsigned count);

// A transform written into a matrix the object owns (e.g. a burn mark).
void renderMatrixFromTransform(RenderMatrix* into, struct Transform* transform, float sceneScale);

// The same, set to identity.
void renderMatrixIdentity(RenderMatrix* into);

// Writes an owned matrix back from the cache for the GE.
void renderMatrixFlush(RenderMatrix* matrix);

// The depth range the render plan slices between stages (16 bits).
#define RENDER_MAX_DEPTH    65535

// Headroom above every slice for depth offsets pulling towards the camera.
#define PSP_DEPTH_HEADROOM  256

// A stage's screen rectangle and depth slice, applied with sceGuViewport,
// sceGuOffset and sceGuDepthRange (the N64 folds it into the projection).
struct RenderViewportRect {
    short minX, minY, maxX, maxY;
    unsigned short minZ, maxZ;
};

RenderViewport renderStateBuildViewport(struct RenderState* renderState, int minX, int minY, int maxX, int maxY, int minZ, int maxZ);

// Re-slices an existing viewport's depth.
void renderViewportSetDepthRange(RenderViewport viewport, int minZ, int maxZ);

// The whole screen.
RenderViewport renderViewportFullscreen();

// Applies the rectangle to the GU (gSPViewport on the N64).
void renderViewportApply(RenderViewport viewport);

#endif
