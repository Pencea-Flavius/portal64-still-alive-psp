#ifndef __LEVELS_N64_INTRO_RENDER_H__
#define __LEVELS_N64_INTRO_RENDER_H__

// The N64 half of the intro's drawing; see src/levels/psp/intro_render.h.
// The logo is drawn from the shared copy.

#include "graphics/renderstate.h"

struct Intro;

// C has no empty structure, hence the field.
struct IntroRender {
    char nothingRetained;
};

void introRenderInit(struct Intro* intro);
void introRender(void* data, struct RenderState* renderState, struct GraphicsTask* task);

#endif
