#include "system/audio.h"

// The PSP half of the audio backend; see src/system/libultra/audio_libultra.c.
//
// Plays sounds.psp (tools/sound/generate_psp_sound_pack.js), raw PCM next to
// EBOOT.PBP, through a software mixer with per voice pitch and pan into the
// hardware SRC channel. The pack is 96MB, so each voice streams a small
// window of its clip from the memory stick.
//
// Whether a sound is playing is counted in game time, as on the N64, since
// cutscenes wait on sounds. No echo; the echo send is still taken out of the
// dry signal so levels match the N64.

#include <math.h>
#include <pspaudio.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/assets/audio/clips.h"
#include "system/psp/audio_mix.h"
#include "system/psp/psp_profile.h"
#include "util/frame_time.h"

#define SOUND_PACK_PATH         "sounds.psp"
#define PSP_MAX_VOICES          32

// Multiple of 64 for the SRC channel. 46ms at 22050Hz; 23ms crackled when
// several voices read the stick in one block.
#define MIX_SAMPLES             1024
#define MIX_BUFFER_COUNT        4
// 370ms per read.
#define VOICE_WINDOW_SAMPLES    8192
#define MIX_THREAD_PRIORITY     0x12
// Read-ahead thread: above the game, below the mixer.
#define READER_THREAD_PRIORITY  0x14
// Blocks a new sound may wait for its first window.
#define START_WAIT_BLOCKS       2

// Master gain in 1/256ths (the N64 levels are quiet on the PSP speaker), with
// a soft knee instead of clipping.
#define MIX_GAIN                512
#define MIX_LIMIT_KNEE          24000

static int16_t mixLimit(int32_t sample) {
    int32_t magnitude = sample < 0 ? -sample : sample;

    if (magnitude > MIX_LIMIT_KNEE) {
        // Above the knee, divide the excess by 1 + excess / room.
        int32_t room = 32767 - MIX_LIMIT_KNEE;
        int32_t excess = magnitude - MIX_LIMIT_KNEE;
        magnitude = MIX_LIMIT_KNEE + (int32_t)(((int64_t)excess * room) / (excess + room));
    }

    return (int16_t)(sample < 0 ? -magnitude : magnitude);
}

struct SoundClip {
    uint32_t offset;
    uint32_t sampleCount;
    uint32_t loopEnd;
};

struct Voice {
    // Owned by the game thread, read by the mixer under sLock.
    int clipId;
    float timeLeft;             // -1 while looping
    float volume;
    float pan;
    float echo;
    int16_t volumeLeft;
    int16_t volumeRight;
    uint32_t step;              // 16.16 samples per output sample
    uint16_t generation;        // bumped on each play, so the mixer restarts
    uint8_t active;
    uint8_t paused;

    // Owned by the mixer.
    uint16_t mixedGeneration;
    uint32_t index;             // whole samples; 16.16 in 32 bits wraps at 3s
    uint32_t fraction;          // 0..0xFFFF
    uint32_t windowStart;
    uint32_t windowLength;
    int16_t* window;
    // Blocks a new sound has waited for its first window, see mixThread().
    uint8_t startWaits;

    // The next window, read ahead by the reader. The mixer requests it only
    // while the reader is idle and takes it once ready.
    int16_t* ahead;
    uint32_t aheadStart;
    uint32_t aheadLength;
    int aheadClip;
    uint16_t aheadGeneration;
    volatile uint8_t aheadState;

    int16_t buffers[2][VOICE_WINDOW_SAMPLES];
};

enum AheadState {
    AheadIdle,
    AheadRequested,
    AheadReading,
    AheadReady,
};

static struct SoundClip* sClips;
static int sClipCount;
static SceUID sPackFile = -1;
// The reader's own file handle, so seeks don't collide.
static SceUID sReaderFile = -1;
static SceUID sReaderWake = -1;
static SceUID sLock = -1;

static struct Voice sVoices[PSP_MAX_VOICES];
static int sVoiceCount;

// Several buffers in turn: the hardware reads one after the output call
// returns (PPSSPP copies, so only real hardware crackled).
static int16_t sMixBuffers[MIX_BUFFER_COUNT][MIX_SAMPLES * 2] __attribute__((aligned(64)));
static int32_t sAccumulator[MIX_SAMPLES * 2];

// Voices whose block is all in their window are mixed on the Media Engine
// (audio_me.c); without it the mix thread does them.
static struct MixJob sJobs[MIX_JOB_MAX] __attribute__((aligned(64)));
static int32_t sMeAccumulator[MIX_SAMPLES * 2] __attribute__((aligned(64)));
static int sMeReady;

struct EdgeVoice {
    struct Voice* voice;
    int clipId;
    int volumeLeft;
    int volumeRight;
    uint32_t step;
};

static struct EdgeVoice sEdges[PSP_MAX_VOICES];

static struct Voice* voiceFor(VoiceId voiceId) {
    if (voiceId < 0 || voiceId >= sVoiceCount || !sVoices[voiceId].active) {
        return 0;
    }

    return &sVoices[voiceId];
}

// Whether the window read ahead for this voice holds `index` of its clip.
static int voiceAheadHas(struct Voice* voice, int clipId, uint32_t index) {
    if (voice->aheadState != AheadReady) {
        return 0;
    }

    __asm__ volatile("" ::: "memory");

    return voice->aheadGeneration == voice->mixedGeneration &&
        voice->aheadClip == clipId &&
        index >= voice->aheadStart &&
        index < voice->aheadStart + voice->aheadLength;
}

// Asks the reader for the next window, or the clip start at a loop.
static void voiceRequestAhead(struct Voice* voice, int clipId, struct SoundClip* clip) {
    if (voice->aheadState == AheadRequested || voice->aheadState == AheadReading || sReaderWake < 0) {
        return;
    }

    uint32_t end = clip->loopEnd ? clip->loopEnd : clip->sampleCount;
    uint32_t next = voice->windowStart + voice->windowLength;

    if (next >= end) {
        if (!clip->loopEnd) {
            return;
        }

        next = 0;
    }

    voice->aheadStart = next;
    voice->aheadClip = clipId;
    voice->aheadGeneration = voice->mixedGeneration;
    __asm__ volatile("" ::: "memory");
    voice->aheadState = AheadRequested;
    sceKernelSignalSema(sReaderWake, 1);
}

static int16_t voiceSample(struct Voice* voice, int clipId, struct SoundClip* clip, uint32_t index) {
    if (index >= clip->sampleCount) {
        return 0;
    }

    if (index < voice->windowStart || index >= voice->windowStart + voice->windowLength) {
        if (voiceAheadHas(voice, clipId, index)) {
            int16_t* played = voice->window;
            voice->window = voice->ahead;
            voice->ahead = played;
            voice->windowStart = voice->aheadStart;
            voice->windowLength = voice->aheadLength;
            voice->aheadState = AheadIdle;
        } else {
            // Not read ahead in time: read it here (may crackle).
            uint32_t count = clip->sampleCount - index;

            if (count > VOICE_WINDOW_SAMPLES) {
                count = VOICE_WINDOW_SAMPLES;
            }

            sceIoLseek32(sPackFile, clip->offset + index * sizeof(int16_t), PSP_SEEK_SET);
            int bytesRead = sceIoRead(sPackFile, voice->window, count * sizeof(int16_t));

            voice->windowStart = index;
            voice->windowLength = bytesRead > 0 ? bytesRead / sizeof(int16_t) : 0;
        }

        voiceRequestAhead(voice, clipId, clip);

        if (voice->windowLength == 0) {
            return 0;
        }
    }

    return voice->window[index - voice->windowStart];
}

// Fills the windows the mixer asked for, one voice at a time.
static int readerThread(SceSize args, void* argp) {
    (void)args;
    (void)argp;

    while (1) {
        sceKernelWaitSema(sReaderWake, 1, 0);

        for (int i = 0; i < sVoiceCount; ++i) {
            struct Voice* voice = &sVoices[i];

            if (voice->aheadState != AheadRequested) {
                continue;
            }

            voice->aheadState = AheadReading;
            __asm__ volatile("" ::: "memory");

            struct SoundClip* clip = &sClips[voice->aheadClip];
            uint32_t start = voice->aheadStart;
            uint32_t count = clip->sampleCount - start;

            if (count > VOICE_WINDOW_SAMPLES) {
                count = VOICE_WINDOW_SAMPLES;
            }

            sceIoLseek32(sReaderFile, clip->offset + start * sizeof(int16_t), PSP_SEEK_SET);
            int bytesRead = sceIoRead(sReaderFile, voice->ahead, count * sizeof(int16_t));

            voice->aheadLength = bytesRead > 0 ? bytesRead / sizeof(int16_t) : 0;
            __asm__ volatile("" ::: "memory");
            voice->aheadState = AheadReady;
        }
    }

    return 0;
}

static void mixVoice(struct Voice* voice, int clipId, int volumeLeft, int volumeRight, uint32_t step) {
    struct SoundClip* clip = &sClips[clipId];
    uint32_t end = clip->loopEnd ? clip->loopEnd : clip->sampleCount;

    // Out of earshot: just advance the voice.
    if (!volumeLeft && !volumeRight) {
        uint64_t advanced = ((uint64_t)voice->fraction + (uint64_t)step * MIX_SAMPLES);
        voice->index += (uint32_t)(advanced >> 16);
        voice->fraction = (uint32_t)(advanced & 0xFFFF);

        if (voice->index >= end && clip->loopEnd) {
            voice->index %= end;
        }

        return;
    }

    for (int i = 0; i < MIX_SAMPLES; ++i) {
        if (voice->index >= end) {
            if (!clip->loopEnd) {
                return;
            }

            voice->index %= end;
        }

        int32_t a;
        int32_t b;
        uint32_t offset = voice->index - voice->windowStart;

        // Fast path: both samples in the window. Edges and loop ends go through
        // voiceSample(), which may read the stick.
        if (voice->index >= voice->windowStart && offset + 1 < voice->windowLength && voice->index + 1 < end) {
            a = voice->window[offset];
            b = voice->window[offset + 1];
        } else {
            uint32_t next = (voice->index + 1 < end || !clip->loopEnd) ? voice->index + 1 : 0;
            a = voiceSample(voice, clipId, clip, voice->index);
            b = voiceSample(voice, clipId, clip, next);
        }

        int32_t sample = a + (((b - a) * (int32_t)voice->fraction) >> 16);

        sAccumulator[i * 2 + 0] += (sample * volumeLeft) >> 15;
        sAccumulator[i * 2 + 1] += (sample * volumeRight) >> 15;

        voice->fraction += step;
        voice->index += voice->fraction >> 16;
        voice->fraction &= 0xFFFF;
    }
}

// An ME job for the voice's block if it needs no window edge, loop or end;
// advances the voice as mixVoice() would. 0 leaves it to mixVoice().
static int mixJobFor(struct Voice* voice, int clipId, int volumeLeft, int volumeRight, uint32_t step, struct MixJob* job) {
    struct SoundClip* clip = &sClips[clipId];
    uint32_t end = clip->loopEnd ? clip->loopEnd : clip->sampleCount;
    uint32_t windowEnd = voice->windowStart + voice->windowLength;
    // The sample after the block's last position, which it interpolates to.
    uint32_t last = voice->index + (uint32_t)(((uint64_t)voice->fraction + (uint64_t)step * (MIX_SAMPLES - 1)) >> 16) + 1;

    if ((!volumeLeft && !volumeRight) || voice->index < voice->windowStart || last >= windowEnd || last >= end) {
        return 0;
    }

    job->samples = voice->window + (voice->index - voice->windowStart);
    job->fraction = voice->fraction;
    job->step = step;
    job->volumeLeft = volumeLeft;
    job->volumeRight = volumeRight;

    uint64_t advanced = (uint64_t)voice->fraction + (uint64_t)step * MIX_SAMPLES;
    voice->index += (uint32_t)(advanced >> 16);
    voice->fraction = (uint32_t)(advanced & 0xFFFF);
    return 1;
}

static int mixThread(SceSize args, void* argp) {
    (void)args;
    (void)argp;

    int nextBuffer = 0;

    while (1) {
        unsigned long long mixStart = pspProfileNow();
        memset(sAccumulator, 0, sizeof(sAccumulator));
        int jobCount = 0;
        int edgeCount = 0;

        for (int i = 0; i < sVoiceCount; ++i) {
            struct Voice* voice = &sVoices[i];

            sceKernelWaitSema(sLock, 1, 0);
            int audible = voice->active && !voice->paused && voice->timeLeft != 0.0f;
            int clipId = voice->clipId;
            int volumeLeft = voice->volumeLeft;
            int volumeRight = voice->volumeRight;
            uint32_t step = voice->step;
            uint16_t generation = voice->generation;
            sceKernelSignalSema(sLock, 1);

            if (!audible) {
                continue;
            }

            if (voice->mixedGeneration != generation) {
                voice->mixedGeneration = generation;
                voice->index = 0;
                voice->fraction = 0;
                voice->windowStart = 0;
                voice->windowLength = 0;
                voice->startWaits = 0;
                // The first window also comes from the reader: the stick is slow right
                // after boot (the Valve jingle).
                voiceRequestAhead(voice, clipId, &sClips[clipId]);
            }

            // Wait a block or two for the first window, then read it here.
            if (voice->windowLength == 0 && voice->index == 0 &&
                !voiceAheadHas(voice, clipId, 0) && voice->startWaits < START_WAIT_BLOCKS) {
                ++voice->startWaits;
                // Ask again in case the reader was busy with the last sound.
                voiceRequestAhead(voice, clipId, &sClips[clipId]);
                continue;
            }

            if (jobCount < MIX_JOB_MAX && mixJobFor(voice, clipId, volumeLeft, volumeRight, step, &sJobs[jobCount])) {
                ++jobCount;
            } else {
                sEdges[edgeCount++] = (struct EdgeVoice){voice, clipId, volumeLeft, volumeRight, step};
            }
        }

        int onMe = sMeReady && jobCount;

        if (onMe) {
            audioMeBegin(sJobs, jobCount, MIX_SAMPLES);
        } else {
            for (int i = 0; i < jobCount; ++i) {
                mixSpan(&sJobs[i], sAccumulator, MIX_SAMPLES);
            }
        }

        for (int i = 0; i < edgeCount; ++i) {
            struct EdgeVoice* edge = &sEdges[i];
            mixVoice(edge->voice, edge->clipId, edge->volumeLeft, edge->volumeRight, edge->step);
        }

        if (onMe) {
            // The wait is the ME's time, not the CPU's.
            pspProfileAdd(PspProfileAudio, mixStart);

            if (!audioMeFinish()) {
                printf("audio: the media engine stopped answering, mixing on the CPU\n");
                sMeReady = 0;

                for (int i = 0; i < jobCount; ++i) {
                    mixSpan(&sJobs[i], sAccumulator, MIX_SAMPLES);
                }

                onMe = 0;
            }

            mixStart = pspProfileNow();
        }

        int16_t* buffer = sMixBuffers[nextBuffer];
        nextBuffer = (nextBuffer + 1) % MIX_BUFFER_COUNT;

        for (int i = 0; i < MIX_SAMPLES * 2; ++i) {
            buffer[i] = mixLimit(((sAccumulator[i] + (onMe ? sMeAccumulator[i] : 0)) * MIX_GAIN) >> 8);
        }

        // The hardware fetches it from memory, not through the CPU's cache.
        sceKernelDcacheWritebackRange(buffer, sizeof(sMixBuffers[0]));
        pspProfileAdd(PspProfileAudio, mixStart);
        sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, buffer);
    }

    return 0;
}

static int loadSoundPack() {
    sPackFile = sceIoOpen(SOUND_PACK_PATH, PSP_O_RDONLY, 0);

    if (sPackFile < 0) {
        return 0;
    }

    uint32_t count;

    // A pack from another build would play the wrong sound for every id.
    if (sceIoRead(sPackFile, &count, sizeof(count)) != sizeof(count) || count != SOUNDS_TOTAL_COUNT) {
        return 0;
    }

    sClips = malloc(count * sizeof(struct SoundClip));

    if (!sClips || sceIoRead(sPackFile, sClips, count * sizeof(struct SoundClip)) != (int)(count * sizeof(struct SoundClip))) {
        return 0;
    }

    sClipCount = count;
    return 1;
}

void* audioInit(void* memoryEnd, int maxVoices) {
    sVoiceCount = maxVoices < PSP_MAX_VOICES ? maxVoices : PSP_MAX_VOICES;

    // Without the pack every sound ends at once, so nothing waits on one.
    if (!loadSoundPack()) {
        printf("audio: %s missing or from another build, no sound\n", SOUND_PACK_PATH);
        sClipCount = 0;
        return memoryEnd;
    }

    sLock = sceKernelCreateSema("audio", 0, 1, 1, 0);

    if (sceAudioSRCChReserve(MIX_SAMPLES, AUDIO_OUTPUT_HZ, 2) < 0) {
        printf("audio: no output channel, no sound\n");
        return memoryEnd;
    }

    for (int i = 0; i < PSP_MAX_VOICES; ++i) {
        sVoices[i].window = sVoices[i].buffers[0];
        sVoices[i].ahead = sVoices[i].buffers[1];
    }

    // Without a reader the mixer reads.
    sReaderFile = sceIoOpen(SOUND_PACK_PATH, PSP_O_RDONLY, 0);
    sReaderWake = sceKernelCreateSema("audio read", 0, 0, 1, 0);
    SceUID reader = sceKernelCreateThread("audio read", readerThread, READER_THREAD_PRIORITY, 0x2000, 0, 0);

    if (sReaderFile < 0 || sReaderWake < 0 || reader < 0 || sceKernelStartThread(reader, 0, 0) < 0) {
        sReaderWake = -1;
    }

    sMeReady = audioMeStart(sMeAccumulator, MIX_SAMPLES);
    printf("audio: mixing on the %s\n", sMeReady ? "media engine and CPU" : "CPU");

    SceUID thread = sceKernelCreateThread("audio", mixThread, MIX_THREAD_PRIORITY, 0x4000, 0, 0);

    if (thread >= 0) {
        sceKernelStartThread(thread, 0, 0);
    }

    return memoryEnd;
}

static void lock() {
    if (sLock >= 0) {
        sceKernelWaitSema(sLock, 1, 0);
    }
}

static void unlock() {
    if (sLock >= 0) {
        sceKernelSignalSema(sLock, 1);
    }
}

void audioUpdate() {
    lock();

    for (int i = 0; i < sVoiceCount; ++i) {
        struct Voice* voice = &sVoices[i];

        if (voice->active && !voice->paused && voice->timeLeft > 0.0f) {
            voice->timeLeft -= FIXED_DELTA_TIME;

            if (voice->timeLeft < 0.0f) {
                voice->timeLeft = 0.0f;
            }
        }
    }

    unlock();
}

void audioDestroy() {
    // Only the ME needs stopping; the system frees the rest on exit.
    if (sMeReady) {
        sMeReady = 0;
        audioMeStop();
    }
}

static void voiceSetParams(struct Voice* voice, float volume, float pitch, float pan, float echo) {
    // A negative value leaves a parameter as it was.
    if (volume >= 0.0f) {
        voice->volume = volume;
    }

    if (pan >= 0.0f) {
        voice->pan = pan;
    }

    if (echo >= 0.0f) {
        voice->echo = echo;
    }

    // As libultra: volume squared, echo send taken from the dry signal, equal
    // power pan.
    float level = voice->volume > 1.0f ? 1.0f : voice->volume;
    float gain = 32767 * level * level * cosf(voice->echo * (float)M_PI_2);

    voice->volumeLeft = gain * cosf(voice->pan * (float)M_PI_2);
    voice->volumeRight = gain * sinf(voice->pan * (float)M_PI_2);

    if (pitch >= 0.0f) {
        voice->step = pitch * 65536.0f;
    }
}

VoiceId audioPlaySound(int soundClipId, float volume, float pitch, float pan, float echo) {

    if (soundClipId < 0 || (sClipCount && soundClipId >= sClipCount)) {
        return VOICE_ID_NONE;
    }

    lock();

    for (int i = 0; i < sVoiceCount; ++i) {
        struct Voice* voice = &sVoices[i];

        if (voice->active) {
            continue;
        }

        float duration = 0.0f;

        if (sClipCount) {
            struct SoundClip* clip = &sClips[soundClipId];

            // Pitch is playback speed, so a higher one ends sooner.
            duration = clip->loopEnd ? -1.0f :
                clip->sampleCount * (1.0f / AUDIO_OUTPUT_HZ) / (pitch > 0.0f ? pitch : 1.0f);
        }

        voice->clipId = soundClipId;
        voice->timeLeft = duration;
        voice->paused = 0;
        voice->active = 1;
        voice->volume = 0.0f;
        voice->pan = 0.5f;
        voice->echo = 0.0f;
        voice->step = 1 << 16;
        ++voice->generation;
        voiceSetParams(voice, volume, pitch, pan, echo);

        unlock();
        return (VoiceId)i;
    }

    unlock();

    // Out of voices; the sound player handles it.
    return VOICE_ID_NONE;
}

void audioSetSoundParams(VoiceId voiceId, float volume, float pitch, float pan, float echo) {

    lock();
    struct Voice* voice = voiceFor(voiceId);

    if (voice) {
        voiceSetParams(voice, volume, pitch, pan, echo);
    }

    unlock();
}

int audioIsSoundPlaying(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);
    return voice && voice->timeLeft != 0.0f;
}

int audioIsSoundLooped(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);
    return voice && voice->timeLeft < 0.0f;
}

void audioPauseSound(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);

    if (voice) {
        voice->paused = 1;
    }
}

int audioIsSoundPaused(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);
    return voice && voice->paused;
}

void audioResumeSound(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);

    if (voice) {
        voice->paused = 0;
    }
}

void audioStopSound(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);

    if (voice) {
        voice->timeLeft = 0.0f;
        voice->paused = 0;
    }
}

void audioReleaseVoice(VoiceId voiceId) {
    struct Voice* voice = voiceFor(voiceId);

    if (voice) {
        voice->active = 0;
    }
}
