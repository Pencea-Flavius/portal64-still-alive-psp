#include "credits.h"

#include "audio/soundplayer.h"
#include "levels.h"
#include "system/controller.h"
#include "system/display.h"
#include "util/frame_time.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/materials/ui.h"

void creditsInit(struct Credits* credits) {
    credits->time = 0.0f;

    soundPlayerPlay(SOUNDS_LOOPING_RADIO_MIX, 0.5f, 1.0f, NULL, NULL, SoundTypeMusic);
}

void creditsUpdate(struct Credits* credits) {
    credits->time += FIXED_DELTA_TIME;

    if (controllerGetButtonsDown(0, ControllerButtonStart)) {
        levelQueueLoad(MAIN_MENU, NULL, NULL, 0);
    }
}

#define FADE_IN_TIME  1.0f

void creditsTextColor(struct Credits* credits, struct Coloru8* color) {
    *color = gColorWhite;

    if (credits->time < FADE_IN_TIME) {
        color->a = (u8)(credits->time * (255.0f / FADE_IN_TIME));
    }

    color->g = 200;
    color->b = 30;
}
