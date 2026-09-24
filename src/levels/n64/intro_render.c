#include "levels/intro.h"

#include "image.h"
#include "system/display.h"

// The tiles are loaded from the shared copy each frame, so there is nothing to
// set up.
void introRenderInit(struct Intro* intro) {
    (void)intro;
}

void introRender(void* data, struct RenderState* renderState, struct GraphicsTask* task) {
    struct Intro* intro = (struct Intro*)data;

    struct Coloru8 fadeColor;
    introFadeColor(intro, &fadeColor);

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_FILL); 
    gDPSetFillColor(renderState->dl++, 0);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD - 1, SCREEN_HT - 1);

    gDPPipeSync(renderState->dl++);
    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE); 
    graphicsCopyImage(
        renderState,
        intro->valveImage,
        VALVE_IMAGE_WIDTH, VALVE_IMAGE_HEIGHT,
        0, 0,
        VALVE_IMAGE_WIDTH, VALVE_IMAGE_HEIGHT,
        (SCREEN_WD - VALVE_IMAGE_WIDTH) / 2, (SCREEN_HT - VALVE_IMAGE_HEIGHT) / 2, 
        fadeColor
    );
}
