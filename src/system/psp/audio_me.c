#include "system/psp/audio_mix.h"

#include <pspiofilemgr.h>
#include <pspkernel.h>

// Mixing on the Media Engine for audio_psp.c. The ME has no kernel, so it
// only gets jobs whose samples are already read. Neither cache sees the
// other's writes: both sides flush and invalidate, and the signal word is
// uncached.
//
// Booted through psp-media-engine-custom-core's kcall.prx (as sf64-psp).
// If that fails or the ME stops answering (e.g. after sleep), the CPU mixes.
// Skipped on PPSSPP (detected via the "kemulator:" devctl), which crashes on
// the first kcall.
#include <me-core-mapper/me-core.h>

#define ME_UNCACHED         0x40000000
// PPSSPP's EMULATOR_DEVCTL__IS_EMULATOR.
#define EMULATOR_IS_EMULATOR 3
#define ME_CACHE_LINE       64
#define ME_READY            0x600D
#define ME_HALTED           0xDEAD
#define ME_STOP_WAIT_US     100000
#define ME_START_WAIT_US    250000
#define ME_FINISH_WAIT_US   20000
#define ME_POLL_US          100

enum MeState {
    MeBooting,
    MeIdle,
    MeRun,
    MeStop,
};

// Own cache line, so no neighbour's write back lands on it.
struct MeShared {
    uint32_t state;
    uint32_t progress;
    const struct MixJob* jobs;
    int32_t count;
    int32_t samples;
    int32_t* accumulator;
    uint32_t pad[10];
};

static struct MeShared sSharedStorage __attribute__((aligned(ME_CACHE_LINE)));

static volatile struct MeShared* meShared() {
    return (volatile struct MeShared*)(ME_UNCACHED | (uint32_t)&sSharedStorage);
}

static uint32_t lineStart(const void* address) {
    return (uint32_t)address & ~(ME_CACHE_LINE - 1);
}

static uint32_t lineLength(const void* address, uint32_t size) {
    return (((uint32_t)address + size + ME_CACHE_LINE - 1) & ~(ME_CACHE_LINE - 1)) - lineStart(address);
}

// The samples a job reads, including the one after its last.
static uint32_t jobBytes(const struct MixJob* job, int samples) {
    return ((((uint64_t)job->fraction + (uint64_t)job->step * (samples - 1)) >> 16) + 2) * sizeof(int16_t);
}

// Runs on the ME.
__attribute__((noinline, aligned(4))) void meLibOnProcess(void) {
    volatile struct MeShared* shared = meShared();

    shared->progress = ME_READY;
    meLibSync();

    while (1) {
        if (shared->state == MeStop) {
            shared->progress = ME_HALTED;
            meLibSync();
            meLibHalt();
        }

        if (shared->state != MeRun) {
            meLibDelayPipeline();
            continue;
        }

        const struct MixJob* jobs = shared->jobs;
        int count = shared->count;
        int samples = shared->samples;
        int32_t* accumulator = shared->accumulator;

        meLibDcacheInvalidateRange(lineStart(jobs), lineLength(jobs, sizeof(struct MixJob) * count));

        for (int i = 0; i < samples * 2; ++i) {
            accumulator[i] = 0;
        }

        for (int i = 0; i < count; ++i) {
            const struct MixJob* job = &jobs[i];
            meLibDcacheInvalidateRange(lineStart(job->samples), lineLength(job->samples, jobBytes(job, samples)));
            mixSpan(job, accumulator, samples);
        }

        meLibDcacheWritebackRange(lineStart(accumulator), lineLength(accumulator, sizeof(int32_t) * 2 * samples));
        meLibSync();
        shared->state = MeIdle;
        meLibSync();
    }
}

int audioMeStart(int32_t* accumulator, int samples) {
    volatile struct MeShared* shared = meShared();

    if (sceIoDevctl("kemulator:", EMULATOR_IS_EMULATOR, NULL, 0, NULL, 0) == 0) {
        return 0;
    }

    // Drop the CPU's cached copies so they don't overwrite the ME's writes.
    sceKernelDcacheWritebackInvalidateRange(&sSharedStorage, sizeof(sSharedStorage));
    sceKernelDcacheWritebackInvalidateRange(accumulator, sizeof(int32_t) * 2 * samples);

    shared->state = MeBooting;
    shared->progress = 0;
    shared->accumulator = accumulator;
    shared->samples = samples;

    if (meLibDefaultInit() < 0) {
        return 0;
    }

    shared->state = MeIdle;

    for (int waited = 0; shared->progress != ME_READY; waited += ME_POLL_US) {
        if (waited >= ME_START_WAIT_US) {
            return 0;
        }

        sceKernelDelayThread(ME_POLL_US);
    }

    return 1;
}

void audioMeBegin(const struct MixJob* jobs, int count, int samplesPerJob) {
    volatile struct MeShared* shared = meShared();

    sceKernelDcacheWritebackRange(jobs, sizeof(struct MixJob) * count);

    for (int i = 0; i < count; ++i) {
        sceKernelDcacheWritebackRange(jobs[i].samples, jobBytes(&jobs[i], samplesPerJob));
    }

    shared->jobs = jobs;
    shared->count = count;
    shared->samples = samplesPerJob;
    __asm__ volatile("sync" ::: "memory");
    shared->state = MeRun;
}

int audioMeFinish() {
    volatile struct MeShared* shared = meShared();

    for (int waited = 0; shared->state != MeIdle; waited += ME_POLL_US) {
        if (waited >= ME_FINISH_WAIT_US) {
            return 0;
        }

        sceKernelDelayThread(ME_POLL_US);
    }

    sceKernelDcacheInvalidateRange(shared->accumulator, sizeof(int32_t) * 2 * shared->samples);
    return 1;
}

// On exit: stop the ME, since its code is in memory the XMB will reuse.
// Lets a block in progress finish first.
void audioMeStop() {
    volatile struct MeShared* shared = meShared();

    for (int waited = 0; waited < ME_STOP_WAIT_US && shared->progress != ME_HALTED; waited += ME_POLL_US) {
        if (shared->state != MeRun) {
            shared->state = MeStop;
        }

        sceKernelDelayThread(ME_POLL_US);
    }
}
