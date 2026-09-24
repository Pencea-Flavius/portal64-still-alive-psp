#include <ultra64.h>

#include <pspthreadman.h>

// The N64 counts CPU cycles here. Nothing in the game converts osGetTime()
// results without going through system/time.h, which the PSP backend also
// reports in microseconds, so the two agree.
OSTime osGetTime(void) {
    return (OSTime)sceKernelGetSystemTimeWide();
}
