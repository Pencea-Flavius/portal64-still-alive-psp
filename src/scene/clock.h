#ifndef __SCENE_CLOCK_H__
#define __SCENE_CLOCK_H__

#include "levels/level_definition.h"
#include "math/transform.h"

struct Clock {
    struct Transform transform;
    short roomIndex;
    float timeLeft;
    short tenThousandths;
};

void clockInit(struct Clock* clock, struct ClockDefinition* definition);
void clockShowMainMenuTime(struct Clock* clock);

void clockUpdate(struct Clock* clock);

// The digits are scrolled per machine; the build puts one of the two on the
// path.
#include "clock_digits.h"

#endif