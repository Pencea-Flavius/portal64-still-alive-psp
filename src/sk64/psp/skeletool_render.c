#include "skeletool_render.h"

#include <pspgum.h>

#include "sk64/skeletool_armature.h"

// The PSP half of the armature's drawing; see src/sk64/n64/skeletool_render.c.
// The pose goes out as float matrices; pspModelDrawSkinned() walks the chain
// from the model's bone table.
void skCalculateTransforms(struct SKArmature* object, RenderMatrix* into) {
    for (int i = 0; i < object->numberOfBones; ++i) {
        renderMatrixFromTransform(&into[i], &object->pose[i], 1.0f);
    }
}

RenderMatrices skArmatureBuildTransforms(struct SKArmature* armature, struct RenderState* renderState) {
    ScePspFMatrix4* result = renderStateRequestMemory(
        renderState, sizeof(ScePspFMatrix4) * armature->numberOfBones);

    if (!result) {
        return NULL;
    }

    skCalculateTransforms(armature, result);

    return result;
}

// Max chain depth, so a broken table cannot loop.
#define SK_MAX_ATTACHMENT_DEPTH 16

RenderMatrices skAttachmentTransform(struct SKArmature* armature, int boneIndex, RenderMatrices objectMatrix, RenderMatrices bones, struct RenderState* renderState) {
    if (!objectMatrix || !bones || boneIndex < 0 || boneIndex >= armature->numberOfBones) {
        return objectMatrix;
    }

    const ScePspFMatrix4* poses = (const ScePspFMatrix4*)bones;
    ScePspFMatrix4* result = renderStateRequestMemory(renderState, sizeof(ScePspFMatrix4));

    if (!result) {
        return NULL;
    }

    // Parent on the left, as in pspModelDrawSkinned().
    ScePspFMatrix4 world = poses[boneIndex];
    int bone = armature->boneParentIndex ? armature->boneParentIndex[boneIndex] : NO_BONE_PARENT;

    for (int depth = 0; bone != NO_BONE_PARENT && bone < armature->numberOfBones && depth < SK_MAX_ATTACHMENT_DEPTH; ++depth) {
        ScePspFMatrix4 product;
        gumMultMatrix(&product, &poses[bone], &world);
        world = product;
        bone = armature->boneParentIndex[bone];
    }

    ScePspFMatrix4 attached;
    gumMultMatrix(&attached, (const ScePspFMatrix4*)objectMatrix, &world);
    *result = attached;

    return result;
}
