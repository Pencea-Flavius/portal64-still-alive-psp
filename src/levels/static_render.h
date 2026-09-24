#ifndef __STATIC_RENDER_H__
#define __STATIC_RENDER_H__

#include "graphics/render_types.h"
#include "graphics/renderstate.h"
#include "level_definition.h"
#include "scene/camera.h"
#include "scene/dynamic_render_list.h"

// Only pointers to a render stage are passed through here. render_plan.h is
// the portal recursion's own header and reaches for the viewport type.
struct RenderProps;

void staticRenderDetermineVisibleRooms(
    struct RenderProps* renderStage,
    struct FrustumCullingInformation* cullingInfo,
    u16 currentRoom,
    u64 nonVisibleRooms,
    u64* coveredDoorways
);
int staticRenderIsRoomVisible(u64 visibleRooms, u16 roomIndex);
void staticRender(
    struct RenderProps* renderStage,
    struct DynamicRenderDataList* dynamicList,
    int stageIndex,
    RenderMatrices staticMatrices,
    struct Transform* staticTransforms,
    struct RenderState* renderState
);

void staticRenderCheckSignalMaterials();

#endif