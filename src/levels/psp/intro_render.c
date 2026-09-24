#include "levels/intro.h"

#include "graphics/psp/psp_render.h"
#include "system/display.h"

#include "codegen/assets/materials/images.h"

// The PSP half of the intro's drawing; see src/levels/n64/intro_render.c.

void introRenderInit(struct Intro* intro) {
    // Nothing to build. intro.c copies the logo out of "ROM" into a buffer
    // for the N64's sake; that copy is a memcpy here and its result is not
    // used, because the source is already the texture this draws.
    (void)intro;
}

void introRender(void* data, struct RenderState* renderState, struct GraphicsTask* task) {
    (void)task;

    struct Intro* intro = (struct Intro*)data;

    struct Coloru8 fadeColor;
    introFadeColor(intro, &fadeColor);

    struct Coloru8 black = {0, 0, 0, 255};
    pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&black));

    // The texture is padded up to a power of two, so the logo is the top left
    // corner of it and the coordinates stop at its own size.
    pspRenderTextureRect(
        renderState, images_material_list[VALVE_INDEX],
        (SCREEN_WD - VALVE_IMAGE_WIDTH) / 2, (SCREEN_HT - VALVE_IMAGE_HEIGHT) / 2,
        VALVE_IMAGE_WIDTH, VALVE_IMAGE_HEIGHT,
        0, 0, VALVE_IMAGE_WIDTH, VALVE_IMAGE_HEIGHT,
        pspRenderColor(&fadeColor)
    );
}
