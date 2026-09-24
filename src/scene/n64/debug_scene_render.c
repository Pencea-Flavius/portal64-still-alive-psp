#include "scene/debug_scene.h"

#include <ultra64.h>

#include "font/liberation_mono.h"
#include "graphics/renderstate.h"
#include "scene/scene.h"
#include "system/display.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/ui.h"

// The N64 half of the debug overlay's drawing. Its counterpart is
// src/scene/psp/debug_scene_render.c. The two strips behind the text and the
// two usage bars are texture rectangles against an env colour here.
void debugSceneRenderBars(struct Scene* scene, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gDPSetCycleType(renderState->dl++, G_CYC_1CYCLE);
    gDPSetFillColor(renderState->dl++, (GPACK_RGBA5551(0, 0, 0, 1) << 16 | GPACK_RGBA5551(0, 0, 0, 1)));
    gDPSetCombineLERP(
        renderState->dl++,
        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT,
        0, 0, 0, ENVIRONMENT, 0, 0, 0, ENVIRONMENT
    );
    gDPSetEnvColor(renderState->dl++, 32, 32, 32, 255);
    gSPTextureRectangle(renderState->dl++, 32 << 2, 32 << 2, (32 + 256) << 2, (32 + 8) << 2, 0, 0, 0, 1, 1);
    gSPTextureRectangle(renderState->dl++, 32 << 2, 44 << 2, (32 + 256) << 2, (44 + 8) << 2, 0, 0, 0, 1, 1);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, 32, 255, 32, 255);

    float cpuUsage = scene->cpuTime / (float)gLastFrameTime;
    gSPTextureRectangle(
        renderState->dl++,
        PERF_METRICS_MARGIN << 2,
        PERF_METRICS_MARGIN << 2,
        (int)(PERF_METRICS_MARGIN + (PERF_BAR_WIDTH * cpuUsage)) << 2,
        (PERF_METRICS_MARGIN + PERF_BAR_HEIGHT) << 2,
        0,
        0, 0,
        1, 1
    );

    float memoryUsage = renderStateMemoryUsage(renderState);
    gSPTextureRectangle(
        renderState->dl++,
        PERF_METRICS_MARGIN << 2,
        (PERF_METRICS_MARGIN + (PERF_BAR_HEIGHT * 2)) << 2,
        (int)(32 + 254 * memoryUsage) << 2,
        (PERF_METRICS_MARGIN + (PERF_BAR_HEIGHT * 3)) << 2,
        0,
        0, 0,
        1, 1
    );
    gDPPipeSync(renderState->dl++);
}

void debugSceneRenderTextMetric(struct FontRenderer* renderer, char* text, int y, struct RenderState* renderState) {
    fontRendererLayout(renderer, &gLiberationMonoFont, text, SCREEN_WD);

    renderState->dl = fontRendererBuildGfx(
        renderer,
        gLiberationMonoImages,
        PERF_METRICS_MARGIN,
        y - renderer->height,
        &gColorWhite,
        renderState->dl
    );
}

