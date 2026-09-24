#ifndef __RENDER_SCENE_H__
#define __RENDER_SCENE_H__

#include "graphics/render_types.h"
#include <ultra64.h>

#include "math/transform.h"
#include "math/plane.h"
#include "renderstate.h"

#define MAX_RENDER_PART_COUNT 256

struct RenderPart {
    RenderMatrices matrix;
    ModelHandle geometry;
    RenderMatrices armature;
#ifdef PSP
    // Moving level geometry, pushed slightly behind fixed geometry on the PSP
    // (16 bit depth). PSP only: the N64's stackMalloc has no room to grow.
    unsigned char isAnimatedLevel;
#endif
};

struct RenderScene {
    u64 visibleRooms;
    struct Plane forwardPlane;
    struct RenderPart* renderParts;
    short* materials;
    int* sortKeys;
    short* renderOrder;
    short* renderOrderCopy;
    int currentRenderPart;
    struct RenderState *renderState;
};

struct RenderScene* renderSceneNew(struct Transform* cameraTransform, struct RenderState *renderState, u64 visibleRooms);
void renderSceneFree(struct RenderScene* renderScene);
void renderSceneAdd(struct RenderScene* renderScene, ModelHandle geometry, RenderMatrices matrix, int materialIndex, struct Vector3* at, RenderMatrices armature);
// Marks the part just added as level geometry that moves. See RenderPart.
void renderSceneMarkLastAnimatedLevel(struct RenderScene* renderScene);

// The sort is shared; renderSceneGenerate() submits it per machine.
void renderSceneSort(struct RenderScene* renderScene, int min, int max);
void renderSceneGenerate(struct RenderScene* renderScene, struct RenderState* renderState);

#endif
