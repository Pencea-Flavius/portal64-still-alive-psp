#ifndef __LEVELS_INTRO_H__
#define __LEVELS_INTRO_H__

#include "../graphics/render_types.h"
#include "../graphics/color.h"
#include "../graphics/renderstate.h"

#define VALVE_IMAGE_WIDTH   160
#define VALVE_IMAGE_HEIGHT  120

#define VALVE_IMAGE_SIZE    (sizeof(u16) * VALVE_IMAGE_WIDTH * VALVE_IMAGE_HEIGHT)

// Everything that draws, and what it keeps, is the platform's half. It is
// included here rather than after the structure because the structure holds
// one of its types by value.
#include "intro_render.h"

struct Intro {
    float time;
    u64* valveImage;
    struct IntroRender render;
};

void introInit(struct Intro* intro);
void introUpdate(struct Intro* intro);

// How faded the logo is at this moment, which is a timeline rather than a
// drawing decision, so both halves ask rather than working it out.
void introFadeColor(struct Intro* intro, struct Coloru8* color);

#endif