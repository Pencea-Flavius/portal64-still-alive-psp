#ifndef __DISPLAY_PSP_H__
#define __DISPLAY_PSP_H__

// Presenting a finished frame has no entry in system/display.h because the N64
// does it from inside its RSP task scheduler. On the PSP it is an explicit call,
// so it lives here rather than being bolted onto the shared contract.
void displayPspSwapBuffers();

// Start and finish a frame's GU display list.
// Bytes left in this frame's display list.
int displayPspListRemaining();

// The VRAM past the depth buffer, which holds textures.
void* displayPspFreeVram(unsigned int* size);

void displayPspStartFrame();
void displayPspEndFrame();

// VRAM offset of the buffer currently being drawn into, as the sceGu buffer
// calls express it. The debug screen needs this to write text into the same
// buffer the GU is building.
unsigned int displayPspGetDrawBufferOffset();

#endif
