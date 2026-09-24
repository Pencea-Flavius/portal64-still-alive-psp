#ifndef __LEVELS_PSP_INTRO_RENDER_H__
#define __LEVELS_PSP_INTRO_RENDER_H__

// The PSP half of the intro's drawing; see src/levels/n64/intro_render.h.
// The logo is drawn straight from its material; nothing is copied.

#include "graphics/renderstate.h"

struct Intro;

// C has no empty structure, hence the field.
struct IntroRender {
    char nothingRetained;
};

void introRenderInit(struct Intro* intro);
void introRender(void* data, struct RenderState* renderState, struct GraphicsTask* task);

#endif
