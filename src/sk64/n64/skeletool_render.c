#include "skeletool_render.h"

#include "sk64/skeletool_defs.h"
#include "system/cartridge.h"
#include "util/memory.h"

Gfx* skBuildAttachments(struct SKArmature* object, Gfx** attachments, struct RenderState* renderState) {
    if (object->numberOfAttachments == 0) {
        return NULL;
    }

    if (object->numberOfAttachments == 1 && attachments) {
        return *attachments;
    }

    Gfx* jumpTable = renderStateAllocateDLChunk(renderState, object->numberOfAttachments);
    Gfx* dl = jumpTable;

    for (unsigned i = 0; i < object->numberOfAttachments; ++i) {
        if (attachments && attachments[i]) {
            gSPBranchList(dl++, attachments[i]);
        } else {
            gSPEndDisplayList(dl++);
        }
    }

    return jumpTable;
}

void skRenderObject(struct SKArmature* object, Gfx** attachements, struct RenderState* intoState) {
    if (!object->displayList) {
        return;
    }

    Mtx* boneMatrices = renderStateRequestMatrices(intoState, object->numberOfBones);

    if (!boneMatrices) {
        return;
    }

    Gfx* jumpTable = skBuildAttachments(object, attachements, intoState);

    if (!jumpTable && object->numberOfAttachments) {
        return;
    }

    if (jumpTable) {
        gSPSegment(intoState->dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(jumpTable));
    }

    skCalculateTransforms(object, boneMatrices);
    gSPSegment(intoState->dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(boneMatrices));
    gSPDisplayList(intoState->dl++, object->displayList);
}

void skCalculateTransforms(struct SKArmature* object, Mtx* into) {
    for (int i = 0; i < object->numberOfBones; ++i) {
        transformToMatrixL(&object->pose[i], &into[i], 1.0f);
    }
}

RenderMatrices skArmatureBuildTransforms(struct SKArmature* armature, struct RenderState* renderState) {
    Mtx* result = renderStateRequestMatrices(renderState, armature->numberOfBones);

    if (!result) {
        return NULL;
    }

    skCalculateTransforms(armature, result);

    return result;
}
