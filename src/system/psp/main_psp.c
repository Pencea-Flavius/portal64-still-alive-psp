#include "audio/soundplayer.h"
#include "controls/controller_actions.h"
#include "controls/rumble_pak_clip.h"
#include "graphics.h"
#include "levels/credits.h"
#include "levels/intro.h"
#include "levels/levels.h"
#include "menu/main_menu.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "scene/portal_surface.h"
#include "scene/scene.h"
#include "strings/translations.h"
#include "system/cartridge.h"
#include "system/controller.h"
#include "system/display.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"
#include "util/memory.h"
#include "util/profile.h"

#include "boot_psp.h"

#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>

#include "psp_profile.h"
#include <pspsysmem.h>

PSP_MODULE_INFO("Portal 64", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

// The C runtime's own heap (TLS, stdio); the game's arena is claimed in
// main().
PSP_HEAP_SIZE_KB(4 * 1024);

// Portal recursion and BVH walks need more than the default stack.
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

// The PSP half of the entry point and main loop; see src/main.c.
// graphicsCreateTask() returns once the frame is on screen, so there are no
// frames in flight to count: update, draw, repeat.

// The game's heap: a partition block from the kernel, trying descending
// sizes (a PSP-1000 has 32MB, later models 64MB).
static void* sGameHeap;
static unsigned sGameHeapSize;

static const unsigned sGameHeapSizes[] = {
    24 * 1024 * 1024,
    16 * 1024 * 1024,
    12 * 1024 * 1024,
     8 * 1024 * 1024,
     4 * 1024 * 1024,
};

struct Scene gScene;
struct GameMenu gGameMenu;
struct Intro gIntro;
struct Credits gCredits;

typedef void (*InitCallback)(void* data);
typedef void (*UpdateCallback)(void* data);

struct SceneCallbacks {
    void* data;
    InitCallback initCallback;
    GraphicsCallback graphicsCallback;
    UpdateCallback updateCallback;
};

struct SceneCallbacks gTestChamberCallbacks = {
    .data = &gScene,
    .initCallback = (InitCallback)&sceneInit,
    .graphicsCallback = (GraphicsCallback)&sceneRender,
    .updateCallback = (UpdateCallback)&sceneUpdate,
};

struct SceneCallbacks gMainMenuCallbacks = {
    .data = &gGameMenu,
    .initCallback = (InitCallback)&mainMenuInit,
    .graphicsCallback = (GraphicsCallback)&mainMenuRender,
    .updateCallback = (UpdateCallback)&mainMenuUpdate,
};

struct SceneCallbacks gIntroCallbacks = {
    .data = &gIntro,
    .initCallback = (InitCallback)&introInit,
    .graphicsCallback = (GraphicsCallback)&introRender,
    .updateCallback = (UpdateCallback)&introUpdate,
};

struct SceneCallbacks gCreditsCallbacks = {
    .data = &gCredits,
    .initCallback = (InitCallback)&creditsInit,
    .graphicsCallback = (GraphicsCallback)&creditsRender,
    .updateCallback = (UpdateCallback)&creditsUpdate,
};

struct SceneCallbacks* gSceneCallbacks = &gTestChamberCallbacks;

void levelLoadWithCallbacks(int levelIndex) {
    if (levelIndex == CREDITS_MENU) {
        gSceneCallbacks = &gCreditsCallbacks;
    } else if (levelIndex == INTRO_MENU) {
        gSceneCallbacks = &gIntroCallbacks;
    } else if (levelIndex == MAIN_MENU) {
        levelLoad(0);
        gSceneCallbacks = &gMainMenuCallbacks;
    } else {
        levelLoad(levelIndex);
        gSceneCallbacks = &gTestChamberCallbacks;
    }

    levelClearQueued();
}

int main() {
    bootPspInit();

    // Homebrew starts at 222MHz; run at the full 333.
    scePowerSetClockFrequency(333, 333, 166);

    displayInit(0 /* the PSP does not interlace */);
    graphicsInit();

    cartridgeInit();
    savefileLoad();

    unsigned int lastUpdateVcount = sceDisplayGetVcount();
    unsigned char inputIgnore = 5;
    unsigned char drawingEnabled = 0;
    unsigned char skippedDraw = 0;
    unsigned drawBufferIndex = 0;

    for (unsigned i = 0; i < sizeof(sGameHeapSizes) / sizeof(*sGameHeapSizes); ++i) {
        SceUID block = sceKernelAllocPartitionMemory(
            PSP_MEMORY_PARTITION_USER, "portal64_heap", PSP_SMEM_Low, sGameHeapSizes[i], NULL);

        if (block >= 0) {
            sGameHeap = sceKernelGetBlockHeadAddr(block);
            sGameHeapSize = sGameHeapSizes[i];
            break;
        }
    }

    if (!sGameHeap) {
        pspDebugScreenInit();
        pspDebugScreenPrintf("Out of memory: no game heap available.\n");
        sceKernelSleepThread();
    }

    void* memoryEnd = soundPlayerInit((unsigned char*)sGameHeap + sGameHeapSize);
    heapInit(sGameHeap, memoryEnd);

    dynamicSceneInit();
    contactSolverInit(&gContactSolver);
    portalSurfaceCleanupQueueInit();

    levelLoadWithCallbacks(INTRO_MENU);
    controllersInit();
    controllerActionInit();
    rumblePakClipInit();
    frameTimeInit(displayGetFPS());
    translationsLoad(gSaveData.video.textLanguage);
    gSceneCallbacks->initCallback(gSceneCallbacks->data);
    // this prevents the intro from crashing
    gGameMenu.currentRenderedLanguage = gSaveData.video.textLanguage;

    while (1) {
        // control the framerate
        //
        // Update every second vblank (30 a second, FIXED_DELTA_TIME each), counted
        // with the vblank counter so a long frame doesn't lose an update.
        unsigned int vcount = sceDisplayGetVcount();

        if (vcount - lastUpdateVcount < FRAME_SKIP + 1) {
            unsigned long long idleStart = pspProfileNow();
            sceDisplayWaitVblankStart();
            pspProfileAdd(PspProfileIdle, idleStart);
            continue;
        }

        // Catch up on an overrun vblank, but not on a long stall like a level load.
        lastUpdateVcount += FRAME_SKIP + 1;

        if (vcount - lastUpdateVcount > 2 * (FRAME_SKIP + 1)) {
            lastUpdateVcount = vcount;
        }

        // Still behind: skip drawing this update to keep game speed at 30 updates,
        // but never twice in a row.
        int skipDraw = !skippedDraw && vcount - lastUpdateVcount >= FRAME_SKIP + 1;
        skippedDraw = skipDraw;

        if (levelGetQueued() != NO_QUEUED_LEVEL) {
            soundPlayerStopAll();
            dynamicSceneInit();
            contactSolverInit(&gContactSolver);
            portalSurfaceRevert(1);
            portalSurfaceRevert(0);
            portalSurfaceCleanupQueueInit();
            heapInit(sGameHeap, memoryEnd);
            translationsLoad(gSaveData.video.textLanguage);
            levelLoadWithCallbacks(levelGetQueued());
            rumblePakClipInit();
            dynamicAssetsReset();
            menuResetDeferredQueue();
            // if a portal fire button is being held
            // don't fire portals until it is released
            controllerActionMuteActive();
            gSceneCallbacks->initCallback(gSceneCallbacks->data);
            continue;
        }

        if (translationsCurrentLanguage() != gGameMenu.currentRenderedLanguage) {
            gameMenuRebuildText(&gGameMenu);
            continue;
        }

        Time startTime = timeGetTime();

        if (drawingEnabled && !skipDraw) {
            graphicsCreateTask(&gGraphicsTasks[drawBufferIndex], gSceneCallbacks->graphicsCallback, gSceneCallbacks->data);
            drawBufferIndex = drawBufferIndex ^ 1;

            // The N64 does these on task done; here the frame is already on screen.
            portalSurfaceCheckCleanupQueue();
            menuTickDeferredQueue();

            if (gScene.checkpointState == SceneCheckpointStatePendingRender) {
                gScene.checkpointState = SceneCheckpointStateReady;
            }

            frameTimeUpdate();
            pspProfileFrame();
        }

        unsigned long long updateStart = pspProfileNow();

        controllersPoll();
        rumblePakClipUpdate();
        controllerActionUpdate();
        romCopyAsyncDrain();

        if (inputIgnore) {
            --inputIgnore;
        } else {
            gSceneCallbacks->updateCallback(gSceneCallbacks->data);
            drawingEnabled = 1;
        }

        soundPlayerUpdate();

        pspProfileAdd(PspProfileUpdate, updateStart);

        profileReport();

        gScene.cpuTime = timeGetTime() - startTime;
    }

    return 0;
}
