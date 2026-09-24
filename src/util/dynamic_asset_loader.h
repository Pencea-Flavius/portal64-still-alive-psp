#ifndef __DYNAMIC_ASSET_LOADER_H__
#define __DYNAMIC_ASSET_LOADER_H__

#include "graphics/render_types.h"

#include <ultra64.h>

#include "dynamic_asset_model.h"
#include "sk64/skeletool_armature.h"
#include "sk64/skeletool_clip.h"

struct SKArmatureWithAnimations {
    struct SKArmatureDefinition* armature;
    struct SKAnimationClip** clips;
    short clipCount;
};

void dynamicAssetsReset();

void dynamicAssetModelPreload(int index);
ModelHandle dynamicAssetModel(int index);

void* dynamicAssetFixPointer(int index, void* ptr);

struct SKArmatureWithAnimations* dynamicAssetAnimatedModel(int index);
struct SKAnimationClip* dynamicAssetClip(int index, int clipIndex);

#endif