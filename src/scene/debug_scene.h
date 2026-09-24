#ifndef __DEBUG_SCENE_H__
#define __DEBUG_SCENE_H__

#include "graphics/renderstate.h"
struct RenderPlan;
#include "scene.h"

#include "font/font.h"
#include "system/display.h"

#define PERF_METRICS_MARGIN      33
#define PERF_METRIC_ROW_PADDING  4
#define PERF_BAR_WIDTH           (SCREEN_WD - (PERF_METRICS_MARGIN * 2))
#define PERF_BAR_HEIGHT          6

// The overlay's two drawing operations. Their signatures are the same on both
// machines and only the bodies differ, so there is no header pair: the two
// live in scene/{n64,psp}/debug_scene_render.c. Everything else about the
// metrics -- what they say and where the rows go -- is shared.
void debugSceneRenderBars(struct Scene* scene, struct RenderState* renderState);
void debugSceneRenderTextMetric(struct FontRenderer* renderer, char* text, int y, struct RenderState* renderState);

void debugSceneInit(struct Scene* scene);
void debugSceneUpdate(struct Scene* scene);
void debugSceneRender(struct Scene* scene, struct RenderState* renderState, struct RenderPlan* renderPlan);

#endif
