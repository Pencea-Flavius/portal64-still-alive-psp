#include "sk64/skeletool_animator.h"

#include "system/cartridge.h"

#include <stdint.h>

// The N64 half of finding a clip's frame data; see src/sk64/psp/skeletool_animation_data.c.
// Clip frames point into the animation segment and are resolved against
// where it was loaded.
extern char _animation_segmentSegmentRomStart[];

const void* skAnimationFrameAt(struct SKAnimationClip* clip, int frameSize, int frame) {
    uint32_t address = (uint32_t)clip->frames + frameSize * frame;

    return CALC_SEGMENT_POINTER(address, _animation_segmentSegmentRomStart);
}
