#ifndef __LEVELS_N64_CREDITS_RENDER_H__
#define __LEVELS_N64_CREDITS_RENDER_H__

// The N64 half of the credits screen's drawing; see src/levels/psp/credits_render.h.

#include "graphics/renderstate.h"

void creditsRender(void* data, struct RenderState* renderState, struct GraphicsTask* task);

#endif
