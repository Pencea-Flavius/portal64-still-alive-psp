#ifndef __INIT_GFX_H__
#define __INIT_GFX_H__

// The RSP and RDP state the frame starts from. N64 only, and no counterpart is
// coming: the GU is set up by sceGuInit and the display half in
// src/system/psp/display_psp.c, which is the same job by a different route.

#include <ultra64.h>

// The whole screen, which renderViewportFullscreen() hands back. Defined
// beside the rest of the state a frame starts from.
extern Vp fullscreenViewport;

extern Gfx setup_rspstate[];
extern Gfx setup_rdpstate[];
extern Gfx rdpstateinit_dl[];

#endif