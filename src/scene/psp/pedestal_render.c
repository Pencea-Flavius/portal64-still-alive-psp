#include "pedestal_render.h"

#include "scene/pedestal.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/pedestal.h"
#include "codegen/assets/models/portal_gun/w_portalgun.h"

// The PSP half of the pedestal's drawing; see src/scene/n64/pedestal_render.c.
// The gun is a second render list entry under its attachment bone.
void pedestalRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Pedestal* pedestal = (struct Pedestal*)data;

    RenderMatrices matrix = renderStateTransformToMatrices(renderState, &pedestal->transform, SCENE_SCALE);

    if (!matrix) {
        return;
    }

    RenderMatrices armature = skArmatureBuildTransforms(&pedestal->armature, renderState);

    if (!armature) {
        return;
    }

    dynamicRenderListAddData(
        renderList,
        pedestal->armature.displayList,
        matrix,
        DEFAULT_INDEX,
        &pedestal->transform.position,
        armature
    );

    if (pedestal->flags & PedestalFlagsDown) {
        return;
    }

    RenderMatrices gunMatrix = skAttachmentTransform(&pedestal->armature, PEDESTAL_ATTACHMENT_GUN_BONE, matrix, armature, renderState);

    if (!gunMatrix) {
        return;
    }

    dynamicRenderListAddData(
        renderList,
        portal_gun_w_portalgun_model_gfx,
        gunMatrix,
        DEFAULT_INDEX,
        &pedestal->transform.position,
        NULL
    );
}
