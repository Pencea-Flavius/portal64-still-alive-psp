#include "scene/debug_scene.h"

#include "font/liberation_mono.h"
#include "graphics/psp/psp_render.h"
#include "graphics/renderstate.h"
#include "scene/scene.h"
#include "system/display.h"
#include "util/frame_time.h"

// The PSP half of the debug overlay's drawing; see src/scene/n64/debug_scene_render.c.
// Strips and bars are plain filled rectangles.

static struct Coloru8 gStripColor = {32, 32, 32, 255};
static struct Coloru8 gBarColor = {32, 255, 32, 255};

void debugSceneRenderBars(struct Scene* scene, struct RenderState* renderState) {
    pspRenderFillRect(renderState, 32, 32, 256, 8, pspRenderColor(&gStripColor));
    pspRenderFillRect(renderState, 32, 44, 256, 8, pspRenderColor(&gStripColor));

    unsigned int barColor = pspRenderColor(&gBarColor);

    float cpuUsage = scene->cpuTime / (float)gLastFrameTime;

    pspRenderFillRect(
        renderState,
        PERF_METRICS_MARGIN,
        PERF_METRICS_MARGIN,
        (int)(PERF_BAR_WIDTH * cpuUsage),
        PERF_BAR_HEIGHT,
        barColor
    );

    float memoryUsage = renderStateMemoryUsage(renderState);

    pspRenderFillRect(
        renderState,
        PERF_METRICS_MARGIN,
        PERF_METRICS_MARGIN + (PERF_BAR_HEIGHT * 2),
        (int)(254 * memoryUsage),
        PERF_BAR_HEIGHT,
        barColor
    );
}

void debugSceneRenderTextMetric(struct FontRenderer* renderer, char* text, int y, struct RenderState* renderState) {
    fontRendererLayout(renderer, &gLiberationMonoFont, text, SCREEN_WD);

    fontRendererDraw(
        renderer,
        gLiberationMonoImages,
        PERF_METRICS_MARGIN,
        y - renderer->height,
        &gColorWhite,
        renderState
    );
}
