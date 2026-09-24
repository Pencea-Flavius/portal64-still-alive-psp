#ifndef __PORTAL_SURFACE_GFX_H__
#define __PORTAL_SURFACE_GFX_H__

// Turns a cut portal surface into drawable geometry. Shared signature, bodies
// in scene/{n64,psp}/portal_surface_gfx.c; vertices in the machine's layout.

#include "graphics/render_types.h"
#include "math/vector3.h"
#include "portal_surface_generator.h"

struct DisplayListResult {
    RenderVertices vtx;
    RenderDisplayList gfx;
};

struct DisplayListResult newGfxFromSurfaceBuilder(struct PortalSurfaceBuilder* surfaceBuilder);

// Size of a run of those vertices.
unsigned portalSurfaceVerticesSize(int count);

// Blends two vertices; the cutter picks which and the weights.
void portalSurfaceVertexLerp(RenderVertices vertices, int aIndex, int bIndex, int resultIndex, float lerp);
void portalSurfaceVertexBarycentric(
    RenderVertices vertices,
    int aIndex, int bIndex, int cIndex, int resultIndex,
    struct Vector3* barycentric
);

#endif
