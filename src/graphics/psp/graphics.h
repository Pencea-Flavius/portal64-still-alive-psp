#ifndef __GRAPHICS_PSP_GRAPHICS_H__
#define __GRAPHICS_PSP_GRAPHICS_H__

// The PSP half of the frame object; see src/graphics/n64/graphics.h.
// The GU owns the display list, so a frame is its scratch and target buffer.
//
// No stencil: 5551 leaves one bit, and 8888 would cost VRAM and fill rate.
// The render plan's scissors and depth clears do without one, as on the N64.

#include "graphics/renderstate.h"

struct GraphicsTask {
    struct RenderState renderState;
    // VRAM offset of the buffer being drawn into.
    unsigned framebuffer;
    unsigned taskIndex;
};

extern struct GraphicsTask gGraphicsTasks[2];

typedef void (*GraphicsCallback)(void* data, struct RenderState* renderState, struct GraphicsTask* task);

void graphicsInit();

// Draws one frame and presents it (the N64 queues a task and returns).
void graphicsCreateTask(struct GraphicsTask* targetTask, GraphicsCallback callback, void* data);

// Clears the depth inside a rectangle, before a portal stage draws.
void graphicsTaskClearZBuffer(struct GraphicsTask* task, int minX, int minY, int maxX, int maxY);

#endif
