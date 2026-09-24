#include "pedestal_render.h"

#include "scene/pedestal.h"
#include "sk64/skeletool_defs.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/portal_gun/w_portalgun.h"

// The N64 half of the pedestal's drawing; see src/scene/psp/pedestal_render.c.
// The gun is spliced in through the bone attachment segment.
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

    Gfx* gunAttachment = portal_gun_w_portalgun_model_gfx;
    Gfx* attachments = skBuildAttachments(&pedestal->armature, (pedestal->flags & PedestalFlagsDown) ? NULL : &gunAttachment, renderState);

    Gfx* objectRender = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = objectRender;

    if (attachments) {
        gSPSegment(dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(attachments));
    }
    gSPSegment(dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(armature));
    gSPDisplayList(dl++, pedestal->armature.displayList);
    gSPEndDisplayList(dl++);

    dynamicRenderListAddData(
        renderList,
        objectRender,
        matrix,
        DEFAULT_INDEX,
        &pedestal->transform.position,
        NULL
    );
}
