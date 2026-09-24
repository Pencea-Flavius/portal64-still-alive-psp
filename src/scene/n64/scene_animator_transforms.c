#include "scene/scene_animator.h"

RenderMatrices sceneAnimatorBuildTransforms(struct SceneAnimator* sceneAnimator, struct RenderState* renderState) {
    Mtx* result = renderStateRequestMatrices(renderState, sceneAnimator->boneCount);

    Mtx* curr = result;

    for (int i = 0; i < sceneAnimator->animatorCount; ++i) {
        skCalculateTransforms(&sceneAnimator->armatures[i], curr);
        curr += sceneAnimator->armatures[i].numberOfBones;
    }
    
    return result;
}
