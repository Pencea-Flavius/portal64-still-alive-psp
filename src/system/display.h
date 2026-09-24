#ifndef __SYSTEM_DISPLAY_H__
#define __SYSTEM_DISPLAY_H__

#include <stdint.h>

#define HIGH_RES 0

#ifdef PSP
    // The PSP's LCD is 480x272 with 512 pixel wide framebuffers: index them by
    // SCREEN_STRIDE, not SCREEN_WD.
    #define SCREEN_WD       480
    #define SCREEN_HT       272
    #define SCREEN_STRIDE   512
#elif HIGH_RES
    #define SCREEN_WD       640
    #define SCREEN_HT       480
    #define SCREEN_STRIDE   SCREEN_WD
#else
    #define SCREEN_WD       320
    #define SCREEN_HT       240
    #define SCREEN_STRIDE   SCREEN_WD
#endif

void displayInit(int interlaced);
void displaySetMode(int interlaced);
void displayClearScreen();

int displayGetFPS();

// Aspect ratio for the projection: a setting on the N64 (4:3 or anamorphic
// 16:9), fixed on the PSP.
float displayGetAspect();
uint16_t* displayGetCurrentFramebuffer();

#endif
