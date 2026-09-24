// Checks mixSpan() (src/system/psp/audio_mix.h), which the Media Engine and
// the CPU run for a block that is all in a voice's window, against the
// per-sample loop in audio_psp.c's mixVoice() it stands in for, and that the
// voice moves on to the same place. Mirrors mixVoice(); update both together.
// Run: cc -I src tools/psp/audio_mix_check.c -o /tmp/amc && /tmp/amc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/psp/audio_mix.h"

#define SAMPLES 1024
#define WINDOW  8192

int main() {
    static int16_t window[WINDOW];
    static int32_t expected[SAMPLES * 2];
    static int32_t actual[SAMPLES * 2];
    int wrong = 0;
    srand(1);

    for (int i = 0; i < WINDOW; ++i) {
        window[i] = (int16_t)(rand() - RAND_MAX / 2);
    }

    for (int run = 0; run < 2000; ++run) {
        uint32_t step = 0x4000 + rand() % 0x20000;
        uint32_t fraction = rand() & 0xFFFF;
        uint32_t index = rand() % (WINDOW / 2);
        int32_t left = rand() % 32768, right = rand() % 32768;
        uint32_t last = index + (uint32_t)(((uint64_t)fraction + (uint64_t)step * (SAMPLES - 1)) >> 16) + 1;

        if (last >= WINDOW) {
            continue;
        }

        memset(expected, 0, sizeof(expected));
        memset(actual, 0, sizeof(actual));

        uint32_t loopIndex = index, loopFraction = fraction;

        for (int i = 0; i < SAMPLES; ++i) {
            int32_t a = window[loopIndex], b = window[loopIndex + 1];
            int32_t sample = a + (((b - a) * (int32_t)loopFraction) >> 16);
            expected[i * 2 + 0] += (sample * left) >> 15;
            expected[i * 2 + 1] += (sample * right) >> 15;
            loopFraction += step;
            loopIndex += loopFraction >> 16;
            loopFraction &= 0xFFFF;
        }

        struct MixJob job = {window + index, fraction, step, left, right};
        mixSpan(&job, actual, SAMPLES);

        uint64_t advanced = (uint64_t)fraction + (uint64_t)step * SAMPLES;

        if (memcmp(expected, actual, sizeof(actual)) ||
            index + (uint32_t)(advanced >> 16) != loopIndex || (advanced & 0xFFFF) != loopFraction) {
            ++wrong;
        }
    }

    printf("wrong blocks: %d\n", wrong);
    return wrong != 0;
}
