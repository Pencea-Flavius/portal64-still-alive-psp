#include "scene/portal_surface_gfx.h"

#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_vertex.h"
#include "math/mathf.h"
#include "scene/portal_surface_generator.h"
#include "util/memory.h"

#include <pspkernel.h>

// The PSP half of turning a cut portal surface into geometry; see
// src/scene/n64/portal_surface_gfx.c. The same triangle walk, emitting an
// index buffer. All portal surfaces are unlit, so PspVertexColor.

// One allocation, since portal_surface.c frees it with a single free().
struct CutSurfaceModel {
    struct PspModel model;
    struct PspModelPart part;
    unsigned short indices[];
};

static void collectTriangles(struct PortalSurfaceBuilder* surfaceBuilder, unsigned short* indices, int* indexCount) {
    for (int edge = 0; edge < surfaceBuilder->currentEdge; ++edge) {
        struct SurfaceEdge* edgePtr = portalSurfaceGetEdge(surfaceBuilder, edge);

        if (edgePtr->nextEdge == NO_EDGE_CONNECTION) {
            continue;
        }

        if (portalSurfaceHasFlag(surfaceBuilder, edge, SurfaceEdgeFlagsFilled)) {
            continue;
        }

        int nextEdge = portalSurfaceNextEdge(surfaceBuilder, edge);
        int prevEdge = portalSurfacePrevEdge(surfaceBuilder, edge);

        portalSurfaceSetFlag(surfaceBuilder, edge, SurfaceEdgeFlagsFilled);
        portalSurfaceSetFlag(surfaceBuilder, nextEdge, SurfaceEdgeFlagsFilled);
        portalSurfaceSetFlag(surfaceBuilder, prevEdge, SurfaceEdgeFlagsFilled);

        // The N64's corner order, so the winding matches.
        indices[(*indexCount)++] = edgePtr->pointIndex;
        indices[(*indexCount)++] = portalSurfaceGetEdge(surfaceBuilder, nextEdge)->pointIndex;
        indices[(*indexCount)++] = portalSurfaceGetEdge(surfaceBuilder, prevEdge)->pointIndex;
    }
}

struct DisplayListResult newGfxFromSurfaceBuilder(struct PortalSurfaceBuilder* surfaceBuilder) {
    // Every triangle closes three edges.
    int maxIndexCount = surfaceBuilder->currentEdge;

    struct CutSurfaceModel* cut = malloc(sizeof(struct CutSurfaceModel) + sizeof(unsigned short) * maxIndexCount);
    unsigned vertexSize = portalSurfaceVerticesSize(surfaceBuilder->currentVertex);
    struct PspVertexColor* vertices = malloc(vertexSize);

    struct DisplayListResult result = {NULL, NULL};

    if (!cut || !vertices) {
        if (cut) {
            free(cut);
        }

        if (vertices) {
            free(vertices);
        }

        return result;
    }

    memCopy(vertices, surfaceBuilder->gfxVertices, vertexSize);

    int indexCount = 0;
    collectTriangles(surfaceBuilder, cut->indices, &indexCount);

    // Use the original wall's material.
    const struct PspModel* original = surfaceBuilder->original ? surfaceBuilder->original->triangles : NULL;
    const struct PspMaterial* material = (original && original->partCount) ? original->parts[0].material : NULL;

    cut->part.vertices = vertices;
    cut->part.indices = cut->indices;
    cut->part.indexCount = (unsigned short)indexCount;
    cut->part.vertexCount = (unsigned short)surfaceBuilder->currentVertex;
    cut->part.vertexFormat = PSP_VERTEX_FORMAT_COLOR;
    cut->part.material = material;
    cut->part.boneIndex = -1;
    cut->part.secondBoneIndex = -1;
    cut->part.vertexBones = NULL;

    cut->model.parts = &cut->part;
    cut->model.partCount = 1;
    cut->model.boneParent = NULL;
    cut->model.boneCount = 0;

    // Written back for the GE.
    sceKernelDcacheWritebackRange(vertices, vertexSize);
    sceKernelDcacheWritebackRange(cut, sizeof(struct CutSurfaceModel) + sizeof(unsigned short) * indexCount);

    result.vtx = vertices;
    result.gfx = &cut->model;
    return result;
}

unsigned portalSurfaceVerticesSize(int count) {
    return sizeof(struct PspVertexColor) * count;
}

static unsigned lerpColor(unsigned a, unsigned b, float t) {
    unsigned result = 0;

    for (int shift = 0; shift < 32; shift += 8) {
        float from = (float)((a >> shift) & 0xFF);
        float to = (float)((b >> shift) & 0xFF);
        unsigned channel = (unsigned)(mathfLerp(from, to, t) + 0.5f);
        result |= (channel > 0xFF ? 0xFF : channel) << shift;
    }

    return result;
}

void portalSurfaceVertexLerp(RenderVertices vertices, int aIndex, int bIndex, int resultIndex, float lerp) {
    struct PspVertexColor* verts = vertices;
    struct PspVertexColor* a = &verts[aIndex];
    struct PspVertexColor* b = &verts[bIndex];
    struct PspVertexColor* result = &verts[resultIndex];

    result->x = (short)mathfLerp(a->x, b->x, lerp);
    result->y = (short)mathfLerp(a->y, b->y, lerp);
    result->z = (short)mathfLerp(a->z, b->z, lerp);

    // Normalised, hence floats.
    result->u = mathfLerp(a->u, b->u, lerp);
    result->v = mathfLerp(a->v, b->v, lerp);

    result->color = lerpColor(a->color, b->color, lerp);
    result->padding = 0;
}

void portalSurfaceVertexBarycentric(
    RenderVertices vertices,
    int aIndex, int bIndex, int cIndex, int resultIndex,
    struct Vector3* barycentric
) {
    struct PspVertexColor* verts = vertices;
    struct PspVertexColor* a = &verts[aIndex];
    struct PspVertexColor* b = &verts[bIndex];
    struct PspVertexColor* c = &verts[cIndex];
    struct PspVertexColor* result = &verts[resultIndex];

    result->x = (short)vector3EvalBarycentric1D(barycentric, a->x, b->x, c->x);
    result->y = (short)vector3EvalBarycentric1D(barycentric, a->y, b->y, c->y);
    result->z = (short)vector3EvalBarycentric1D(barycentric, a->z, b->z, c->z);

    result->u = vector3EvalBarycentric1D(barycentric, a->u, b->u, c->u);
    result->v = vector3EvalBarycentric1D(barycentric, a->v, b->v, c->v);

    unsigned color = 0;

    for (int shift = 0; shift < 32; shift += 8) {
        float channel = vector3EvalBarycentric1D(
            barycentric,
            (float)((a->color >> shift) & 0xFF),
            (float)((b->color >> shift) & 0xFF),
            (float)((c->color >> shift) & 0xFF)
        ) + 0.5f;

        unsigned value = channel < 0.0f ? 0 : (unsigned)channel;
        color |= (value > 0xFF ? 0xFF : value) << shift;
    }

    result->color = color;
    result->padding = 0;
}
