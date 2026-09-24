#ifndef _SKELETOOL_ANIMATOR_H
#define _SKELETOOL_ANIMATOR_H

#include "math/transform.h"
#include "skeletool_clip.h"

enum SKAnimatorFlags {
    SKAnimatorFlagsLoop = (1 << 0),
    SKAnimatorFlagsDone = (1 << 1),
};

struct SKAnimator {
    struct SKAnimationClip* currentClip;
    float currentTime;
    float blendLerp;
    struct SKAnimationBoneFrame* boneState[2];
    short boneStateFrames[2];
    short nextFrameStateIndex;
    short flags;
    short nBones;
};

void skAnimatorInit(struct SKAnimator* animator, int nBones);
void skAnimatorCleanup(struct SKAnimator* animator);
void skAnimatorUpdate(struct SKAnimator* animator, struct Transform* transforms, float deltaTime);

void skAnimatorRunClip(struct SKAnimator* animator, struct SKAnimationClip* clip, float startTime, int flags);
void skAnimatorEnsureClipRunning(struct SKAnimator* animator, struct SKAnimationClip* clip, float startTime, int flags);
int skAnimatorIsRunning(struct SKAnimator* animator);

struct SKAnimatorBlender {
    struct SKAnimator from;
    struct SKAnimator to;

    float blendLerp;
};

void skBlenderInit(struct SKAnimatorBlender* blender, int nBones);
void skBlenderCleanup(struct SKAnimatorBlender* animator);
void skBlenderUpdate(struct SKAnimatorBlender* blender, struct Transform* transforms, float deltaTime);

// Where one frame of a clip's bone data is. A clip's frames pointer is a
// segmented address on the N64, resolved against the animation segment the
// linker placed; on the PSP the animations are linked and it is already a
// pointer. Which of those it is is the machine's, so the build picks the half.
const void* skAnimationFrameAt(struct SKAnimationClip* clip, int frameSize, int frame);

#endif
