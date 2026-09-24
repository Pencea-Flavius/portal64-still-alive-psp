#include "boot_psp.h"

#include <ultra64.h>

#include <pspkernel.h>

#define CALLBACK_THREAD_PRIORITY    0x11
#define CALLBACK_THREAD_STACK_SIZE  0xFA0

// The game's, which puts the Media Engine away (audio_psp.c); the renderer
// test that shares this file has no audio and gets this empty one.
__attribute__((weak)) void audioDestroy() {
}

static int bootExitCallback(int arg1, int arg2, void* common) {
    audioDestroy();
    sceKernelExitGame();
    return 0;
}

// Exit callbacks are only delivered to a thread parked in a callback-aware
// wait, so this thread exists purely to sit in one.
static int bootCallbackThread(SceSize args, void* argp) {
    int callbackId = sceKernelCreateCallback("portal64_exit", bootExitCallback, NULL);

    if (callbackId >= 0) {
        sceKernelRegisterExitCallback(callbackId);
    }

    sceKernelSleepThreadCB();
    return 0;
}

void bootPspInit(void) {
    int threadId = sceKernelCreateThread(
        "portal64_callbacks",
        bootCallbackThread,
        CALLBACK_THREAD_PRIORITY,
        CALLBACK_THREAD_STACK_SIZE,
        PSP_THREAD_ATTR_USER,
        NULL
    );

    if (threadId >= 0) {
        sceKernelStartThread(threadId, 0, NULL);
    }

    osInitialize();
}
