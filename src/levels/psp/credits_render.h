#ifndef __LEVELS_PSP_CREDITS_RENDER_H__
#define __LEVELS_PSP_CREDITS_RENDER_H__

// The PSP half of the credits screen's drawing; see src/levels/n64/credits_render.h.

#include "graphics/renderstate.h"

void creditsRender(void* data, struct RenderState* renderState, struct GraphicsTask* task);

#endif
