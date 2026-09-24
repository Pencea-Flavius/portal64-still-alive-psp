
#ifndef _ASSERT_H
#define _ASSERT_H

// Game code says portalAssert. On the N64 it must be __assert (libultra's
// assert calls it); on the PSP newlib's __assert would collide.
#if NDEBUG
    #define portalAssert(assertion)
#else
    #ifdef PSP
        void portalAssert(int assertion);
    #else
        void __assert(int assertion);
        #define portalAssert(assertion) __assert(assertion)
    #endif
#endif

#endif