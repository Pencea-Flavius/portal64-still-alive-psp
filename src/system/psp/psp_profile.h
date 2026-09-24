#ifndef __PSP_PROFILE_H__
#define __PSP_PROFILE_H__

// Frame time breakdown. The profiler (VideoSaveFlagsProfiler, no menu entry)
// draws it and writes it every second to
// ms0:/PSP/SAVEDATA/PORTAL64/perf.txt.

#include "graphics/renderstate.h"

enum PspProfileBin {
    // The game's own update: physics, player, cutscenes, sound player.
    PspProfileUpdate,
    // Working out what each view sees: rooms, portals, the render plan.
    PspProfilePlan,
    // GE commands for the main view, portal view and portal-in-portal view,
    // CPU clipping included.
    PspProfileView0,
    PspProfileView1,
    PspProfileView2,
    // The portal gun, the HUD, the menus.
    PspProfileOverlay,
    // Waiting for the GE to finish drawing after the CPU is done with it.
    PspProfileGpuWait,
    // Waiting for the vertical blank: time the frame did not need.
    PspProfileIdle,

    // Per part timers, only with the profiler on. scene is staticRender();
    // draw is sort and submit; bind and clip are inside draw.
    PspProfileScene,
    PspProfileDraw,
    PspProfileBind,
    PspProfileClip,
    // Inside draw: matrix stack, draw calls, recolour copies.
    PspProfileMatrix,
    PspProfileSubmit,
    PspProfileCopy,
    // CPU skinning, once per part a frame.
    PspProfileSkin,
    // The portal gun, inside overlay.
    PspProfileGun,
    // Inside draw: sorting triangles of parts crossing an edge, and sorting a
    // view's parts.
    PspProfileCut,
    PspProfileSort,
    // The profiler's own drawing and bookkeeping, kept out of overlay.
    PspProfileSelf,
    // Sound mixing, on a higher priority thread, so it is hidden in the others.
    PspProfileAudio,
    PspProfileBinCount,
};

enum PspProfileCounter {
    PspProfileStages,
    PspProfileParts,
    // Parts found entirely off screen by the clipper and skipped.
    PspProfilePartsCulled,
    // Vertices the CPU transformed to test against the screen edges.
    PspProfileVertices,
    PspProfileTriangles,
    // Triangles that crossed a screen edge and were cut on the CPU.
    PspProfileTrianglesClipped,
    // Of those: near, far, and guard band crossings (may overlap).
    PspProfileTrianglesClippedNear,
    PspProfileTrianglesClippedFar,
    PspProfileTrianglesClippedBand,
    // Texture changes.
    PspProfileTextureBinds,
    // Material binds.
    PspProfileMaterialBinds,
    // Binds skipped as the material already bound.
    PspProfileMaterialBindsSkipped,
    PspProfileDrawCalls,
    // Parts drawn from a recoloured copy of their vertices.
    PspProfileCopies,
    // Parts drawn in portal and portal-in-portal views.
    PspProfilePartsView1,
    PspProfilePartsView2,
    // Skinned parts computed, and reused.
    PspProfileSkinned,
    PspProfileSkinReused,
    PspProfileCounterCount,
};

#include <pspkernel.h>

// Owned by the renderer so it links without the rest of the profiler.
extern unsigned long long gPspProfileBins[PspProfileBinCount];
extern unsigned gPspProfileCounters[PspProfileCounterCount];
// Whether the finer, per part timers run.
extern int gPspProfileDetail;

static inline unsigned long long pspProfileNow() {
    return sceKernelGetSystemTimeWide();
}

static inline void pspProfileAdd(enum PspProfileBin bin, unsigned long long since) {
    gPspProfileBins[bin] += pspProfileNow() - since;
}

static inline void pspProfileCount(enum PspProfileCounter counter, int amount) {
    gPspProfileCounters[counter] += amount;
}

// The per part timers: nothing unless the profiler is on.
static inline unsigned long long pspProfileDetailNow() {
    return gPspProfileDetail ? pspProfileNow() : 0;
}

static inline void pspProfileDetailAdd(enum PspProfileBin bin, unsigned long long since) {
    if (gPspProfileDetail) {
        pspProfileAdd(bin, since);
    }
}

// A log line for an event (checkpoint saved or loaded).
void pspProfileNote(const char* format, ...);

// Once per frame drawn, after it is on screen.
void pspProfileFrame();

// The overlay, drawn last in the frame.
void pspProfileRender(struct RenderState* renderState);

#endif
