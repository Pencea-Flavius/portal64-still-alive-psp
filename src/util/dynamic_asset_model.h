#ifndef __DYNAMIC_ASSET_MODEL_H__
#define __DYNAMIC_ASSET_MODEL_H__

#include "graphics/render_types.h"

#include <ultra64.h>

#include "sk64/skeletool_armature.h"
#include "sk64/skeletool_clip.h"

// The segment fields are the N64's, where dynamic models are copied from
// ROM on demand. The PSP links every model.
struct DynamicAssetModel {
#ifndef PSP
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
#endif
    ModelHandle model;
    char* name;
};

// N64 only, as above.
struct DynamicAnimatedAssetModel {
#ifndef PSP
    void* addressStart;
    void* addressEnd;
    void* segmentStart;
#endif
    struct SKArmatureDefinition* armature;
    struct SKAnimationClip** clips;
    short clipCount;
    char* name;
};

#endif
