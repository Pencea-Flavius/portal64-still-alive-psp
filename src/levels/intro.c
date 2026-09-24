#include "intro.h"

#include "audio/soundplayer.h"
#include "levels.h"
#include "system/cartridge.h"
#include "system/controller.h"
#include "system/display.h"
#include "util/frame_time.h"
#include "util/memory.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/materials/images.h"

#define INTRO_TIME  9.0f

#define FADE_IN_TIME  1.0f
#define IMAGE_END_TIME 7.0f
#define FADE_OUT_TIME   1.0f

void introInit(struct Intro* intro) {
    intro->time = 0.0f;

    intro->valveImage = malloc(VALVE_IMAGE_SIZE);
    romCopy((char*)images_valve_rgba_16b, (char*)intro->valveImage, VALVE_IMAGE_SIZE);

    introRenderInit(intro);

    soundPlayerPlay(SOUNDS_VALVE, 1.0f, 1.0f, NULL, NULL, SoundTypeMusic);
}

void introUpdate(struct Intro* intro) {
    intro->time += FIXED_DELTA_TIME;

    if (intro->time > INTRO_TIME || controllerGetButtonsDown(0, ControllerButtonStart)) {
        levelQueueLoad(MAIN_MENU, NULL, NULL, 0);
    }
}

void introFadeColor(struct Intro* intro, struct Coloru8* color) {
    *color = gColorWhite;

    if (intro->time < FADE_IN_TIME) {
        color->a = (u8)((255.0f / FADE_IN_TIME) * intro->time);
    } else if (intro->time > IMAGE_END_TIME) {
        color->a = 0;
    } else if (intro->time > IMAGE_END_TIME - FADE_OUT_TIME) {
        color->a = (u8)((255.0f / FADE_OUT_TIME) * (IMAGE_END_TIME - intro->time));
    }
}
