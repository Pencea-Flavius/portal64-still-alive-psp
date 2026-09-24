#include "scene/scene_animator.h"

#include <pspgum.h>

// The PSP half of building the animated bone transforms for a frame; see
// src/scene/n64/scene_animator_transforms.c.
RenderMatrices sceneAnimatorBuildTransforms(struct SceneAnimator* sceneAnimator, struct RenderState* renderState) {
    ScePspFMatrix4* result = renderStateRequestMemory(
        renderState, sizeof(ScePspFMatrix4) * sceneAnimator->boneCount);

    if (!result) {
        return NULL;
    }

    ScePspFMatrix4* curr = result;

    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        skCalculateTransforms(&sceneAnimator->armatures[i], curr);
        curr += sceneAnimator->armatures[i].numberOfBones;
    }

    return result;
}
