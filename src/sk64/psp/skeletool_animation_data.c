#include "sk64/skeletool_animator.h"

// The PSP half of finding a clip's frame data; see src/sk64/n64/skeletool_animation_data.c.
// Clips are linked, so a frame is an offset into the frames pointer.
const void* skAnimationFrameAt(struct SKAnimationClip* clip, int frameSize, int frame) {
    return (const char*)clip->frames + frameSize * frame;
}
