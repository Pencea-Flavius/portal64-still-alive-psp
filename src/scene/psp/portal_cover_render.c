#include "scene/portal_render.h"

#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "graphics/psp/psp_vertex.h"
#include "scene/render_plan.h"
#include "graphics.h"
#include "system/display.h"

#include <pspgu.h>
#include <pspgum.h>

#include "codegen/assets/models/portal/portal_blue_filled.h"
#include "codegen/assets/models/portal/portal_orange_filled.h"

// The PSP half of the portal cover's drawing; see src/scene/n64/portal_cover_render.c.

// Drawn when the camera is in a portal's plane: the screen area the portal
// covers, depth only at the nearest value (65535) so this stage can't draw
// over the view through it. The polygon is clipped to the screen first;
// its corners can be thousands of pixels off screen.
#define SCREEN_COVER_MAX_POINTS (MAX_NEAR_POLYGON_SIZE + 4)

struct CoverPoint {
    float x, y;
};

// Inside when positive: 0 left, 1 right, 2 top, 3 bottom.
static float coverEdgeDistance(const struct CoverPoint* p, int edge) {
    switch (edge) {
        case 0: return p->x;
        case 1: return SCREEN_WD - p->x;
        case 2: return p->y;
        default: return SCREEN_HT - p->y;
    }
}

static int coverClipEdge(const struct CoverPoint* in, int count, struct CoverPoint* out, int edge) {
    int outCount = 0;

    for (int i = 0; i < count && outCount + 2 <= SCREEN_COVER_MAX_POINTS; ++i) {
        const struct CoverPoint* a = &in[i];
        const struct CoverPoint* b = &in[(i + 1) % count];
        float da = coverEdgeDistance(a, edge);
        float db = coverEdgeDistance(b, edge);

        if (da >= 0.0f) {
            out[outCount++] = *a;
        }

        if ((da >= 0.0f) != (db >= 0.0f)) {
            float t = da / (da - db);
            out[outCount].x = a->x + (b->x - a->x) * t;
            out[outCount].y = a->y + (b->y - a->y) * t;
            ++outCount;
        }
    }

    return outCount;
}

void portalRenderScreenCover(struct Vector2s16* points, int pointCount, struct RenderProps* props, struct RenderState* renderState) {
    (void)props;

    if (pointCount < 3 || pointCount > MAX_NEAR_POLYGON_SIZE) {
        return;
    }

    struct CoverPoint bufferA[SCREEN_COVER_MAX_POINTS];
    struct CoverPoint bufferB[SCREEN_COVER_MAX_POINTS];

    // The screen clipper's units: quarter pixels, y up from the bottom.
    for (int i = 0; i < pointCount; ++i) {
        bufferA[i].x = points[i].x * 0.25f;
        bufferA[i].y = SCREEN_HT - points[i].y * 0.25f;
    }

    struct CoverPoint* in = bufferA;
    struct CoverPoint* out = bufferB;
    int count = pointCount;

    for (int edge = 0; edge < 4 && count >= 3; ++edge) {
        count = coverClipEdge(in, count, out, edge);

        struct CoverPoint* swap = in;
        in = out;
        out = swap;
    }

    if (count < 3) {
        return;
    }

    int triangleCount = count - 2;

    struct PspVertexColor* vertices = renderStateRequestMemory(
        renderState, sizeof(struct PspVertexColor) * triangleCount * 3);

    if (!vertices) {
        return;
    }

    struct PspVertexColor* vertex = vertices;

    for (int i = 2; i < count; ++i) {
        const struct CoverPoint* corner[3] = {&in[0], &in[i - 1], &in[i]};

        for (int c = 0; c < 3; ++c) {
            vertex->u = 0.0f;
            vertex->v = 0.0f;
            vertex->color = 0;
            vertex->x = (short)(corner[c]->x + 0.5f);
            vertex->y = (short)(corner[c]->y + 0.5f);
            // The near end (depth is reversed).
            vertex->z = (short)0xFFFF;
            vertex->padding = 0;
            ++vertex;
        }
    }

    // Depth only, always written.
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_CULL_FACE);
    sceGuPixelMask(0xFFFFFFFF);
    sceGuDisable(GU_BLEND);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_ALWAYS);
    sceGuDepthMask(0);
    pspMaterialForgetState();

    // The format must include the struct's unused UVs, or the GE misreads it.
    sceGuDrawArray(
        GU_TRIANGLES,
        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
        triangleCount * 3,
        NULL,
        vertices
    );

    // Restore defaults; the next bind sets the rest.
    sceGuDepthFunc(GU_GEQUAL);
    sceGuPixelMask(0);
}

void portalRenderCover(struct Portal* portal, float portalTransform[4][4], struct RenderState* renderState) {
    RenderMatrices matrix = renderStateMatrixFromFloat(renderState, portalTransform);

    if (!matrix) {
        return;
    }

    const struct PspModel* model = (portal->flags & PortalFlagsOddParity)
        ? &portal_portal_blue_filled_model
        : &portal_portal_orange_filled_model;

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumMultMatrix((const ScePspFMatrix4*)matrix);

    // Pulled off the wall by the caller, like the rim; see
    // portalRimDepthBias() in render_plan_execute.c.
    pspModelDraw(model);

    sceGumPopMatrix();
}
