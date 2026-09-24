#include "psp_profile.h"

#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "font/font.h"
#include "font/liberation_mono.h"
#include "graphics/color.h"
#include "levels/levels.h"
#include "savefile/savefile.h"
#include "system/display.h"
#include "util/memory.h"

#define PROFILE_LOG_PATH        "ms0:/PSP/SAVEDATA/PORTAL64/perf.txt"
#define PROFILE_WINDOW_US       1000000ull
#define PROFILE_TEXT_X          4
#define PROFILE_TEXT_Y          4
#define PROFILE_LINE_COUNT      10
#define PROFILE_LINE_LENGTH     80

// Frames slower than this get their own line; slower than the load
// threshold is a level load.
#define PROFILE_SPIKE_US            40000ull
#define PROFILE_LOAD_US             150000ull
#define PROFILE_SPIKES_PER_WINDOW   5

static unsigned sFrames;
static unsigned long long sWindowStart;
static unsigned long long sLastFrame;
static unsigned long long sWorstFrame;
static int sLoggedLevel = -1;

// Totals at the end of the last frame, to extract one frame's breakdown.
static unsigned long long sFrameStartBins[PspProfileBinCount];
static unsigned sFrameStartCounters[PspProfileCounterCount];
static int sSpikesThisWindow;

// The per part timers slow the game, so they run every other second:
// detail 0 is real speed, detail 1 the breakdown.
static int sDetailWindow;

// The last complete second, shown by the overlay.
static char sLines[PROFILE_LINE_COUNT][PROFILE_LINE_LENGTH];
static int sLineCount;
static int sGpuLine;
static float sFps;

static float perFrameMs(unsigned long long us) {
    return sFrames ? (float)us / (1000.0f * sFrames) : 0.0f;
}

static unsigned perFrame(unsigned count) {
    return sFrames ? count / sFrames : 0;
}

static unsigned long long difference(unsigned long long a, unsigned long long b) {
    return a > b ? a - b : 0;
}

// The log is written by a low priority thread; writing from the game
// thread caused the stutters it measured. Single producer, single consumer
// ring.
#define PROFILE_RING_SIZE       32768
#define PROFILE_WRITER_PRIORITY 0x60

static char sRing[PROFILE_RING_SIZE];
static volatile unsigned sRingHead;     // written by the game
static volatile unsigned sRingTail;     // written by the writer
static SceUID sWriterWake = -1;
static int sSessionStarted;

// Opened and closed every time: the file length is only recorded on close,
// and exiting with HOME closes nothing.
static int profileWriterThread(SceSize args, void* argp) {
    (void)args;
    (void)argp;

    static char chunk[4096];

    while (1) {
        sceKernelWaitSema(sWriterWake, 1, 0);

        while (sRingTail != sRingHead) {
            unsigned length = 0;

            while (sRingTail != sRingHead && length < sizeof(chunk)) {
                chunk[length++] = sRing[sRingTail];
                sRingTail = (sRingTail + 1) % PROFILE_RING_SIZE;
            }

            sceIoMkdir("ms0:/PSP/SAVEDATA", 0777);
            sceIoMkdir("ms0:/PSP/SAVEDATA/PORTAL64", 0777);
            SceUID log = sceIoOpen(PROFILE_LOG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);

            if (log >= 0) {
                sceIoWrite(log, chunk, length);
                sceIoClose(log);
            }
        }
    }

    return 0;
}

static void profileQueue(const char* text) {
    for (const char* c = text; *c; ++c) {
        unsigned next = (sRingHead + 1) % PROFILE_RING_SIZE;

        // Full: drop the rest.
        if (next == sRingTail) {
            break;
        }

        sRing[sRingHead] = *c;
        sRingHead = next;
    }
}

static void profileWrite(const char* text) {
    if (sWriterWake < 0) {
        sWriterWake = sceKernelCreateSema("perf log", 0, 0, 1, 0);
        SceUID writer = sceKernelCreateThread("perf log", profileWriterThread, PROFILE_WRITER_PRIORITY, 0x2000, 0, 0);

        if (sWriterWake < 0 || writer < 0) {
            return;
        }

        sceKernelStartThread(writer, 0, 0);
    }

    if (!sSessionStarted) {
        sSessionStarted = 1;
        profileQueue(
            "\n# ---- session, build " __DATE__ " " __TIME__ " ----\n"
            "# One line a second. Times are milliseconds per frame, averaged over the second;\n"
            "# worst is the slowest single frame. view0 is the main view, view1 the view through\n"
            "# a portal, view2 through a portal seen through a portal: the CPU building the GE's\n"
            "# commands for it. Inside the views: cull chooses what to draw, draw submits it --\n"
            "# bind (materials), clip (edge tests), matrix (matrix stack), submit (draw calls),\n"
            "# skin (moving skinned vertices by their bones), copy (recoloured vertices), rest.\n"
            "# self is the profiler's own drawing and logging, left out of overlay.\n"
            "# gpu wait is the GE still drawing after the CPU finished; idle is waiting for the\n"
            "# screen; audio is the mixing thread, which interrupts all of the above.\n"
            "# parts a/b/c are the main view's, view1's and view2's. skinned a/b: skinned\n"
            "# parts worked out, and drawn again from that in another view.\n"
            "# A 'spike' line is one frame slower than 40ms, with its own breakdown; lines are\n"
            "# written by a thread of their own, so logging costs the game next to nothing.\n");
    }

    profileQueue(text);
    sceKernelSignalSema(sWriterWake, 1);
}

static int profileLogging() {
    return (gSaveData.video.flags & VideoSaveFlagsProfiler) != 0;
}

static void profileSummarise(unsigned long long now) {
    unsigned long long* bins = gPspProfileBins;
    unsigned* counters = gPspProfileCounters;
    float seconds = (float)(now - sWindowStart) / 1000000.0f;

    sFps = seconds > 0.0f ? sFrames / seconds : 0.0f;

    unsigned partsView1 = perFrame(counters[PspProfilePartsView1]);
    unsigned partsView2 = perFrame(counters[PspProfilePartsView2]);
    unsigned parts = perFrame(counters[PspProfileParts]);

    sLineCount = 0;
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "%.1f fps  worst %.1f ms  depth %d",
        sFps, (float)sWorstFrame / 1000.0f, gSaveData.gameplay.portalRenderDepth);
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "update %.1f  plan %.1f  overlay %.1f  gun %.1f",
        perFrameMs(bins[PspProfileUpdate]), perFrameMs(bins[PspProfilePlan]),
        perFrameMs(bins[PspProfileOverlay]), perFrameMs(bins[PspProfileGun]));
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "view0 %.1f  view1 %.1f  view2 %.1f",
        perFrameMs(bins[PspProfileView0]), perFrameMs(bins[PspProfileView1]), perFrameMs(bins[PspProfileView2]));

    if (gPspProfileDetail) {
        unsigned long long measured = bins[PspProfileBind] + bins[PspProfileClip] + bins[PspProfileMatrix] +
            bins[PspProfileSubmit] + bins[PspProfileCopy] + bins[PspProfileSkin] +
            bins[PspProfileCut] + bins[PspProfileSort];

        snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "cull %.1f  draw %.1f  bind %.1f  clip %.1f",
            perFrameMs(difference(bins[PspProfileScene], bins[PspProfileDraw])), perFrameMs(bins[PspProfileDraw]),
            perFrameMs(bins[PspProfileBind]), perFrameMs(bins[PspProfileClip]));
        snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "matrix %.1f submit %.1f skin %.1f copy %.1f cut %.1f sort %.1f rest %.1f",
            perFrameMs(bins[PspProfileMatrix]), perFrameMs(bins[PspProfileSubmit]), perFrameMs(bins[PspProfileSkin]),
            perFrameMs(bins[PspProfileCopy]), perFrameMs(bins[PspProfileCut]), perFrameMs(bins[PspProfileSort]),
            perFrameMs(difference(bins[PspProfileDraw], measured)));
    }

    sGpuLine = sLineCount;
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "gpu wait %.1f  idle %.1f  audio %.1f  self %.1f",
        perFrameMs(bins[PspProfileGpuWait]), perFrameMs(bins[PspProfileIdle]), perFrameMs(bins[PspProfileAudio]),
        perFrameMs(bins[PspProfileSelf]));
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "stages %u  parts %u/%u/%u  culled %u",
        perFrame(counters[PspProfileStages]), parts - partsView1 - partsView2, partsView1, partsView2,
        perFrame(counters[PspProfilePartsCulled]));
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "verts %u tris %u clipped %u near %u far %u band %u",
        perFrame(counters[PspProfileVertices]), perFrame(counters[PspProfileTriangles]), perFrame(counters[PspProfileTrianglesClipped]), perFrame(counters[PspProfileTrianglesClippedNear]),
        perFrame(counters[PspProfileTrianglesClippedFar]), perFrame(counters[PspProfileTrianglesClippedBand]));
    snprintf(sLines[sLineCount++], PROFILE_LINE_LENGTH, "draws %u  mat %u/%u  tex %u  copies %u  skinned %u/%u",
        perFrame(counters[PspProfileDrawCalls]), perFrame(counters[PspProfileMaterialBinds]),
        perFrame(counters[PspProfileMaterialBindsSkipped]),
        perFrame(counters[PspProfileTextureBinds]), perFrame(counters[PspProfileCopies]),
        perFrame(counters[PspProfileSkinned]), perFrame(counters[PspProfileSkinReused]));

    if (profileLogging()) {
        char line[PROFILE_LINE_COUNT * (PROFILE_LINE_LENGTH + 3) + 32];
        int length = snprintf(line, sizeof(line), "level %d detail %d |", gCurrentLevelIndex, sDetailWindow);

        for (int i = 0; i < sLineCount && length < (int)sizeof(line); ++i) {
            length += snprintf(line + length, sizeof(line) - length, " %s |", sLines[i]);
        }

        if (length < (int)sizeof(line) - 1) {
            line[length++] = '\n';
            line[length] = '\0';
        }

        profileWrite(line);
    }

    memset(gPspProfileBins, 0, sizeof(gPspProfileBins));
    memset(gPspProfileCounters, 0, sizeof(gPspProfileCounters));
    sFrames = 0;
    sWorstFrame = 0;
    sSpikesThisWindow = 0;
    sWindowStart = now;
    sDetailWindow = !sDetailWindow;
}

void pspProfileNote(const char* format, ...) {
    if (!profileLogging()) {
        return;
    }

    char line[256];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(line, sizeof(line) - 1, format, args);
    va_end(args);

    if (length < 0) {
        return;
    }

    if (length > (int)sizeof(line) - 2) {
        length = sizeof(line) - 2;
    }

    line[length++] = '\n';
    line[length] = '\0';
    profileWrite(line);
}

static void pspProfileFrameWork(unsigned long long now);

// One frame's own breakdown, for a frame much slower than the rest.
static void profileSpike(unsigned long long frameUs) {
    unsigned long long bin[PspProfileBinCount];
    unsigned counter[PspProfileCounterCount];

    for (int i = 0; i < PspProfileBinCount; ++i) {
        bin[i] = difference(gPspProfileBins[i], sFrameStartBins[i]);
    }

    for (int i = 0; i < PspProfileCounterCount; ++i) {
        counter[i] = gPspProfileCounters[i] - sFrameStartCounters[i];
    }

    char line[384];
    snprintf(line, sizeof(line),
        "spike %.1f ms level %d | update %.1f plan %.1f view0 %.1f view1 %.1f view2 %.1f overlay %.1f gpu %.1f idle %.1f audio %.1f"
        " | cull %.1f bind %.1f clip %.1f matrix %.1f submit %.1f | stages %u parts %u/%u/%u draws %u\n",
        frameUs / 1000.0f, gCurrentLevelIndex,
        bin[PspProfileUpdate] / 1000.0f, bin[PspProfilePlan] / 1000.0f,
        bin[PspProfileView0] / 1000.0f, bin[PspProfileView1] / 1000.0f, bin[PspProfileView2] / 1000.0f,
        bin[PspProfileOverlay] / 1000.0f, bin[PspProfileGpuWait] / 1000.0f, bin[PspProfileIdle] / 1000.0f,
        bin[PspProfileAudio] / 1000.0f,
        difference(bin[PspProfileScene], bin[PspProfileDraw]) / 1000.0f,
        bin[PspProfileBind] / 1000.0f, bin[PspProfileClip] / 1000.0f, bin[PspProfileMatrix] / 1000.0f,
        bin[PspProfileSubmit] / 1000.0f,
        counter[PspProfileStages],
        counter[PspProfileParts] - counter[PspProfilePartsView1] - counter[PspProfilePartsView2],
        counter[PspProfilePartsView1], counter[PspProfilePartsView2],
        counter[PspProfileDrawCalls]);
    profileWrite(line);
}

void pspProfileFrame() {
    unsigned long long now = pspProfileNow();
    pspProfileFrameWork(now);
    pspProfileAdd(PspProfileSelf, now);
}

static void pspProfileFrameWork(unsigned long long now) {
    gPspProfileDetail = profileLogging() && sDetailWindow;

    if (!sWindowStart) {
        sWindowStart = now;
        sLastFrame = now;
        return;
    }

    ++sFrames;

    unsigned long long frameUs = now - sLastFrame;

    if (frameUs > sWorstFrame) {
        sWorstFrame = frameUs;
    }

    if (profileLogging()) {
        if (gCurrentLevelIndex != sLoggedLevel) {
            char line[48];
            snprintf(line, sizeof(line), "# level %d\n", gCurrentLevelIndex);
            profileWrite(line);
            sLoggedLevel = gCurrentLevelIndex;
        }

        if (frameUs > PROFILE_SPIKE_US && frameUs < PROFILE_LOAD_US && sSpikesThisWindow < PROFILE_SPIKES_PER_WINDOW) {
            profileSpike(frameUs);
            ++sSpikesThisWindow;
        }
    }

    sLastFrame = now;

    if (now - sWindowStart >= PROFILE_WINDOW_US) {
        profileSummarise(now);
    }

    memcpy(sFrameStartBins, gPspProfileBins, sizeof(sFrameStartBins));
    memcpy(sFrameStartCounters, gPspProfileCounters, sizeof(sFrameStartCounters));
}

void pspProfileRender(struct RenderState* renderState) {
    int full = (gSaveData.video.flags & VideoSaveFlagsProfiler) != 0;

    if (!sLineCount || !(full || (gSaveData.video.flags & VideoSaveFlagsShowFps))) {
        return;
    }

    struct FontRenderer* renderer = stackMalloc(sizeof(struct FontRenderer));
    char fps[16];
    int y = PROFILE_TEXT_Y;

    // On screen only the summary; ten lines of text cost milliseconds.
    static const int lines[] = {0, 1, 2, -1};
    int lineCount = full ? 4 : 1;

    for (int n = 0; n < lineCount; ++n) {
        int i = lines[n] < 0 ? sGpuLine : lines[n];
        char* text = sLines[i];

        if (!full) {
            snprintf(fps, sizeof(fps), "%.0f fps", sFps);
            text = fps;
        }

        fontRendererLayout(renderer, &gLiberationMonoFont, text, SCREEN_WD);
        fontRendererDraw(renderer, gLiberationMonoImages, PROFILE_TEXT_X, y, &gColorWhite, renderState);
        y += renderer->height;
    }

    stackMallocFree(renderer);
}
