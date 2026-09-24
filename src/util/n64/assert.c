
#include "util/assert.h"

// The N64 half of the assert; see src/util/psp/assert.c.
// teq traps into the debugger when the assertion is zero (MIPS II; the PSP's
// Allegrex does not have it).

#if !(NDEBUG)

asm(
".global __assert\n"
".balign 4\n"
"__assert:\n"
    "teq $a0, $0\n"
    "jr $ra\n"
    "nop\n"
);

#endif