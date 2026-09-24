#include "system/display.h"

#include "display_psp.h"

#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspkernel.h>

#include "util/frame_time.h"

#define PSP_REFRESH_RATE    60

// 5551 like the N64, leaving ~1.2MB of VRAM for textures.
#define DISPLAY_PIXEL_FORMAT    GU_PSM_5551
#define DISPLAY_BYTES_PER_PIXEL 2
#define DEPTH_BYTES_PER_PIXEL   2

#define FRAMEBUFFER_BYTES   (SCREEN_STRIDE * SCREEN_HT * DISPLAY_BYTES_PER_PIXEL)
#define DEPTHBUFFER_BYTES   (SCREEN_STRIDE * SCREEN_HT * DEPTH_BYTES_PER_PIXEL)

// Offsets into VRAM, which is what the sceGu buffer calls expect.
#define FRAMEBUFFER_0_OFFSET    0
#define FRAMEBUFFER_1_OFFSET    (FRAMEBUFFER_0_OFFSET + FRAMEBUFFER_BYTES)
#define DEPTHBUFFER_OFFSET      (FRAMEBUFFER_1_OFFSET + FRAMEBUFFER_BYTES)

// One fixed display list. pspgu never checks its end, so the renderer stops
// short itself (displayPspListRemaining()); sized so it shouldn't have to.
#define DISPLAY_LIST_SIZE   (2 * 1024 * 1024)

static unsigned int __attribute__((aligned(16))) sDisplayList[DISPLAY_LIST_SIZE / sizeof(unsigned int)];

// Which buffer is displayed, for displayGetCurrentFramebuffer().
static unsigned int sDisplayedOffset = FRAMEBUFFER_1_OFFSET;

static void* vramPointer(unsigned int offset) {
    return (void*)((unsigned int)sceGeEdramGetAddr() + offset);
}

void displayInit(int interlaced) {
    sceGuInit();

    sceGuStart(GU_DIRECT, sDisplayList);

    sceGuDrawBuffer(DISPLAY_PIXEL_FORMAT, (void*)FRAMEBUFFER_0_OFFSET, SCREEN_STRIDE);
    sceGuDispBuffer(SCREEN_WD, SCREEN_HT, (void*)FRAMEBUFFER_1_OFFSET, SCREEN_STRIDE);
    sceGuDepthBuffer((void*)DEPTHBUFFER_OFFSET, SCREEN_STRIDE);

    // The GU works in a 4096x4096 space with the screen centred in it.
    sceGuOffset(2048 - (SCREEN_WD / 2), 2048 - (SCREEN_HT / 2));
    sceGuViewport(2048, 2048, SCREEN_WD, SCREEN_HT);

    // PSP depth runs backwards compared to most hardware.
    sceGuDepthRange(65535, 0);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);

    sceGuScissor(0, 0, SCREEN_WD, SCREEN_HT);
    sceGuEnable(GU_SCISSOR_TEST);

    // Counter-clockwise, as Blender and assimp export.
    sceGuFrontFace(GU_CCW);
    sceGuEnable(GU_CULL_FACE);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CLIP_PLANES);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void displaySetMode(int interlaced) {
    // The LCD is progressive only; the menu entry should be hidden on PSP.
}

void displayClearScreen() {
    displayPspStartFrame();

    sceGuClearColor(0);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    displayPspEndFrame();
    displayPspSwapBuffers();
}

int displayGetFPS() {
    return PSP_REFRESH_RATE;
}

float displayGetAspect() {
    // From the panel (480x272), not the widescreen setting, or it stretches.
    return (float)SCREEN_WD / (float)SCREEN_HT;
}

// The displayed frame copied by the GE for the CPU to read: emulators keep
// VRAM stale for the CPU (save thumbnails came out black in PPSSPP).
static uint16_t __attribute__((aligned(64))) sCaptureBuffer[SCREEN_STRIDE * SCREEN_HT];
static unsigned int __attribute__((aligned(16))) sCaptureList[64];

uint16_t* displayGetCurrentFramebuffer() {
    // Flush first so the cache can't overwrite the copy.
    sceKernelDcacheWritebackInvalidateRange(sCaptureBuffer, sizeof(sCaptureBuffer));

    // Between frames: the last one ended with sceGuSync, so the GE is idle.
    sceGuStart(GU_DIRECT, sCaptureList);
    sceGuCopyImage(
        DISPLAY_PIXEL_FORMAT,
        0, 0, SCREEN_WD, SCREEN_HT, SCREEN_STRIDE, vramPointer(sDisplayedOffset),
        0, 0, SCREEN_STRIDE, sCaptureBuffer
    );
    sceGuTexSync();
    sceGuFinish();
    sceGuSync(0, 0);

    // Uncached: the GE wrote it.
    return (uint16_t*)(0x40000000 | (unsigned int)sCaptureBuffer);
}

unsigned int displayPspGetDrawBufferOffset() {
    return (sDisplayedOffset == FRAMEBUFFER_0_OFFSET)
        ? FRAMEBUFFER_1_OFFSET
        : FRAMEBUFFER_0_OFFSET;
}

void* displayPspFreeVram(unsigned int* size) {
    unsigned int used = DEPTHBUFFER_OFFSET + DEPTHBUFFER_BYTES;
    *size = sceGeEdramGetSize() - used;
    return vramPointer(used);
}

int displayPspListRemaining() {
    return DISPLAY_LIST_SIZE - sceGuCheckList();
}

void displayPspStartFrame() {
    sceGuStart(GU_DIRECT, sDisplayList);
}

void displayPspEndFrame() {
    sceGuFinish();
    sceGuSync(0, 0);
}

// The vblank the last frame went on screen at.
static unsigned int sLastSwapVcount;

void displayPspSwapBuffers() {
    // Every frame stays up for FRAME_SKIP + 1 vblanks; waiting only for the
    // next one caused uneven frame times.
    do {
        sceDisplayWaitVblankStart();
    } while ((int)(sceDisplayGetVcount() - sLastSwapVcount) < FRAME_SKIP + 1);

    sLastSwapVcount = sceDisplayGetVcount();

    // Whatever was being drawn becomes what is displayed.
    sDisplayedOffset = (sDisplayedOffset == FRAMEBUFFER_0_OFFSET)
        ? FRAMEBUFFER_1_OFFSET
        : FRAMEBUFFER_0_OFFSET;

    sceGuSwapBuffers();
}
