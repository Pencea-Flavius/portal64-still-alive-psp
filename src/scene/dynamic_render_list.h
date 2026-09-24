#ifndef __SCENE_DYNAMIC_RENDER_LIST_H__
#define __SCENE_DYNAMIC_RENDER_LIST_H__

#include "graphics/render_types.h"
#include <ultra64.h>

#include "graphics/render_scene.h"
#include "math/vector3.h"

// Only a pointer to a render stage is held here. render_plan.h is the portal
// recursion's own header and reaches for the viewport type.
struct RenderProps;

struct DynamicRenderData {
    ModelHandle model;
    RenderMatrices transform;
    struct Vector3 position;
    RenderMatrices armature;
    short materialIndex;
    short renderStageCullingMask;
};

struct DynamicRenderDataList {
    struct RenderState* renderState;
    struct DynamicRenderData* renderData;
    short maxLength;
    short currentLength;
    short currentRenderStateCullingMask;
    short renderStageCount;
    struct RenderProps* renderStages;
    float portalTransforms[2][4][4];
};

struct DynamicRenderDataList* dynamicRenderListNew(struct RenderState* renderState, struct RenderProps* renderStages, int renderStageCount, int maxLength);
void dynamicRenderListFree(struct DynamicRenderDataList* list);

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    ModelHandle model,
    RenderMatrices transform,
    short materialIndex,
    struct Vector3* position,
    RenderMatrices armature
);

void dynamicRenderListAddDataTouchingPortal(
    struct DynamicRenderDataList* list,
    ModelHandle model,
    RenderMatrices transform,
    short materialIndex,
    struct Vector3* position,
    RenderMatrices armature,
    int rigidBodyFlags
);

void dynamicRenderListPopulate(struct DynamicRenderDataList* list);
void dynamicRenderPopulateRenderScene(
    struct DynamicRenderDataList* list,
    int stageIndex,
    struct RenderScene* renderScene
);

#endif
