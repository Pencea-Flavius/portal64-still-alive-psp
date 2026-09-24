#ifndef __AUDIO_MIX_H__
#define __AUDIO_MIX_H__

#include <stdint.h>

// One voice's block, all of whose samples are already in memory: the mixer's
// inner loop with nothing left in it that can read the memory stick, so either
// processor can run it.
struct MixJob {
    const int16_t* samples;     // the first sample the block reads
    uint32_t fraction;          // 0..0xFFFF
    uint32_t step;              // 16.16
    int32_t volumeLeft;
    int32_t volumeRight;
};

#define MIX_JOB_MAX 32

static inline void mixSpan(const struct MixJob* job, int32_t* accumulator, int samples) {
    const int16_t* source = job->samples;
    uint32_t fraction = job->fraction;
    uint32_t position = 0;

    for (int i = 0; i < samples; ++i) {
        int32_t a = source[position];
        int32_t b = source[position + 1];
        int32_t sample = a + (((b - a) * (int32_t)fraction) >> 16);

        accumulator[i * 2 + 0] += (sample * job->volumeLeft) >> 15;
        accumulator[i * 2 + 1] += (sample * job->volumeRight) >> 15;

        fraction += job->step;
        position += fraction >> 16;
        fraction &= 0xFFFF;
    }
}

// The Media Engine, which mixes jobs while the CPU does the rest. Start is
// once, at init; zero if there is no ME to use (PPSSPP, or it did not answer).
int audioMeStart(int32_t* accumulator, int samples);
// Hands it the jobs, which it adds into the accumulator it was started with,
// cleared first. The jobs and the samples they read must stay put until
// audioMeFinish(), which returns zero if the ME stopped answering; it is not
// asked again after that.
void audioMeBegin(const struct MixJob* jobs, int count, int samplesPerJob);
int audioMeFinish();
// Halts the ME for good; see audio_me.c.
void audioMeStop();

#endif
