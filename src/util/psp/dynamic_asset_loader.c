#include "util/dynamic_asset_loader.h"

#include "util/memory.h"

#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/dynamic_model_list.h"

// The PSP half of the dynamic asset loader; see src/util/n64/dynamic_asset_loader.c.
// Every model is linked, so nothing is loaded or relocated.

// Returned for an out of range index, so callers need no null check.
static const struct PspModel gBlankModel = {
    NULL,
    0,
};

static struct SKArmatureDefinition gBlankArmature = {
    &gBlankModel,
    NULL,
    NULL,
    0,
    0,
};

static struct SKArmatureWithAnimations gBlankArmatureWithAnimations = {
    &gBlankArmature,
    NULL,
    0,
};

static struct SKAnimationClip gBlankClip = {
    0,
    0,
    NULL,
    0,
};

// An entry from the generated list, in the shape the game asks for.
static struct SKArmatureWithAnimations gAnimatedModels[DYNAMIC_ANIMATED_MODEL_COUNT];

void dynamicAssetsReset() {
    zeroMemory(gAnimatedModels, sizeof(gAnimatedModels));
}

void dynamicAssetModelPreload(int index) {
    // Everything is already resident.
    (void)index;
}

ModelHandle dynamicAssetModel(int index) {
    if (index < 0 || index >= DYNAMIC_MODEL_COUNT) {
        return &gBlankModel;
    }

    return gDynamicModels[index].model;
}

void* dynamicAssetFixPointer(int index, void* ptr) {
    // Nothing was copied, so nothing moved.
    (void)index;
    return ptr;
}

struct SKArmatureWithAnimations* dynamicAssetAnimatedModel(int index) {
    if (index < 0 || index >= DYNAMIC_ANIMATED_MODEL_COUNT) {
        return &gBlankArmatureWithAnimations;
    }

    struct SKArmatureWithAnimations* result = &gAnimatedModels[index];

    if (!result->armature) {
        struct DynamicAnimatedAssetModel* model = &gDynamicAnimatedModels[index];

        result->armature = model->armature;
        result->clips = model->clips;
        result->clipCount = model->clipCount;
    }

    return result;
}

struct SKAnimationClip* dynamicAssetClip(int index, int clipIndex) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(index);

    if (clipIndex < 0 || clipIndex >= armature->clipCount) {
        return &gBlankClip;
    }

    return armature->clips[clipIndex];
}
