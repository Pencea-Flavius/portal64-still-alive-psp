#include "system/time.h"

#include <psptypes.h>
#include <pspthreadman.h>

// The N64 backend counts CPU cycles. Time is opaque to the game outside this
// file, so the PSP uses the unit its own clock reports: microseconds.
#define MICROSECONDS_PER_SECOND     1000000
#define NANOSECONDS_PER_MICROSECOND 1000

Time timeGetTime() {
    return (Time)sceKernelGetSystemTimeWide();
}

Time timeFromSeconds(float seconds) {
    return (Time)(seconds * MICROSECONDS_PER_SECOND);
}

uint64_t timeMicroseconds(Time time) {
    return (uint64_t)time;
}

uint64_t timeNanoseconds(Time time) {
    return (uint64_t)time * NANOSECONDS_PER_MICROSECOND;
}
