#ifndef __LEVELS_CREDITS_H__
#define __LEVELS_CREDITS_H__

#include "../graphics/render_types.h"
#include "../graphics/color.h"
#include "../graphics/renderstate.h"

struct Credits {
    float time;
};

void creditsInit(struct Credits* credits);
void creditsUpdate(struct Credits* credits);
// How faded the text is at this moment, which is a timeline rather than a
// drawing decision, so both halves ask rather than working it out.
void creditsTextColor(struct Credits* credits, struct Coloru8* color);

// Drawing is declared in the platform's half, which the build puts on the
// include path.
#include "credits_render.h"

#endif