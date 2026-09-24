#include "util/assert.h"

#if !(NDEBUG)

#include <pspdebug.h>
#include <pspkernel.h>

// The PSP half of the assert; see src/util/n64/assert.c.
// The Allegrex has no teq, so a failed assertion is shown on screen and
// stops.
void portalAssert(int assertion) {
    if (assertion) {
        return;
    }

    pspDebugScreenInit();
    pspDebugScreenPrintf("Assertion failed.\n");

    sceKernelSleepThread();
}

#endif
