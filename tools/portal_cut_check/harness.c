// Runs the portal cutter over every portal surface of every level and
// checks the result: every triangle faces the wall's way, and the area is the
// wall's minus the portal's.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "scene/portal_surface.h"
#include "scene/portal_surface_generator.h"
#include "scene/portal_surface_gfx.h"
#include "scene/portal.h"
#include "math/quaternion.h"
#include "math/transform.h"

#include "surfaces.h"

// What newGfxFromSurfaceBuilder() would draw: the same walk as both halves.
static int sTri[1024][3];
static int sTriCount;

struct DisplayListResult newGfxFromSurfaceBuilder(struct PortalSurfaceBuilder* b) {
    sTriCount = 0;
    for (int edge = 0; edge < b->currentEdge; ++edge) {
        struct SurfaceEdge* e = portalSurfaceGetEdge(b, edge);
        if (e->nextEdge == NO_EDGE_CONNECTION || portalSurfaceHasFlag(b, edge, SurfaceEdgeFlagsFilled)) continue;
        int n = portalSurfaceNextEdge(b, edge), p = portalSurfacePrevEdge(b, edge);
        portalSurfaceSetFlag(b, edge, SurfaceEdgeFlagsFilled);
        portalSurfaceSetFlag(b, n, SurfaceEdgeFlagsFilled);
        portalSurfaceSetFlag(b, p, SurfaceEdgeFlagsFilled);
        sTri[sTriCount][0] = e->pointIndex;
        sTri[sTriCount][1] = portalSurfaceGetEdge(b, n)->pointIndex;
        sTri[sTriCount][2] = portalSurfaceGetEdge(b, p)->pointIndex;
        ++sTriCount;
    }
    struct DisplayListResult r = {malloc(1), malloc(1)};
    return r;
}
unsigned portalSurfaceVerticesSize(int count) { return 32 * count; }
void portalSurfaceVertexLerp(RenderVertices v, int a, int b, int r, float l) {}
void portalSurfaceVertexBarycentric(RenderVertices v, int a, int b, int c, int r, struct Vector3* bc) {}

static double tri(struct Vector2s16* v, int a, int b, int c) {
    return 0.5 * ((double)(v[b].x - v[a].x) * (v[c].y - v[a].y) - (double)(v[c].x - v[a].x) * (v[b].y - v[a].y));
}

// The original wall's area and winding, from its own faces.
static double surfaceArea(struct PortalSurface* s, int* sign) {
    double area = 0;
    char* seen = calloc(s->edgeCount, 1);
    for (int i = 0; i < s->edgeCount; ++i) {
        if (seen[i]) continue;
        struct SurfaceEdge* e = &s->edges[i];
        seen[i] = seen[e->nextEdge] = seen[e->prevEdge] = 1;
        area += tri(s->vertices, e->pointIndex, s->edges[e->nextEdge].pointIndex, s->edges[e->prevEdge].pointIndex);
    }
    free(seen);
    *sign = area < 0 ? -1 : 1;
    return fabs(area);
}

static int onSurface(struct PortalSurface* s, struct Vector2s16* p) {
    for (int i = 0; i < s->edgeCount; ++i) {
        struct SurfaceEdge* e = &s->edges[i];
        struct Vector2s16* v[3] = {&s->vertices[e->pointIndex], &s->vertices[s->edges[e->nextEdge].pointIndex], &s->vertices[s->edges[e->prevEdge].pointIndex]};
        int pos = 0, neg = 0;
        for (int k = 0; k < 3; ++k) {
            struct Vector2s16* a = v[k]; struct Vector2s16* b = v[(k + 1) % 3];
            long long c = (long long)(b->x - a->x) * (p->y - a->y) - (long long)(b->y - a->y) * (p->x - a->x);
            pos |= c > 0; neg |= c < 0;
        }
        if (!(pos && neg)) return 1;
    }
    return 0;
}

static double loopArea(struct Vector2s16* l) {
    double a = 0;
    for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
        struct Vector2s16* p = &l[i]; struct Vector2s16* q = &l[(i + 1) % PORTAL_LOOP_SIZE];
        a += (double)p->x * q->y - (double)q->x * p->y;
    }
    return fabs(a) * 0.5;
}

int main(int argc, char** argv) {
    int verbose = argc > 1;
    setvbuf(stdout, NULL, _IOLBF, 0);
    long cuts = 0, failed = 0, flipped = 0, missing = 0;
    long refusedAt[11] = {0};
    long refusedOutside = 0;
    int badSurfaces = 0;

    int only = getenv("ONLY") ? atoi(getenv("ONLY")) : -1;
    int trace = getenv("TRACE") != NULL;
    for (int si = 0; si < SURFACE_COUNT; ++si) {
        if (only >= 0 && si != only) continue;
        struct PortalSurface* s = &sSurfaces[si];
        s->gfxVertices = calloc(s->vertexCount, 32);
        fprintf(stderr, "\r%d ", si);
        int sign;
        double wall = surfaceArea(s, &sign);
        int surfaceBad = 0;

        struct Vector2s16 mn = s->vertices[0], mx = s->vertices[0];
        for (int i = 1; i < s->vertexCount; ++i) {
            if (s->vertices[i].x < mn.x) mn.x = s->vertices[i].x;
            if (s->vertices[i].y < mn.y) mn.y = s->vertices[i].y;
            if (s->vertices[i].x > mx.x) mx.x = s->vertices[i].x;
            if (s->vertices[i].y > mx.y) mx.y = s->vertices[i].y;
        }

        struct Vector3 normal;
        vector3Cross(&s->right, &s->up, &normal);

        for (int gx = 1; gx < 8; ++gx) for (int gy = 1; gy < 8; ++gy) for (int roll = 0; roll < 2; ++roll) {
            struct Vector2s16 at = {{{mn.x + (mx.x - mn.x) * gx / 8, mn.y + (mx.y - mn.y) * gy / 8}}};
            struct Transform portalAt;
            portalSurfaceInverse(s, &at, &portalAt.position);
            float angle = roll * 0.7f;
            struct Vector3 up, tmp1, tmp2;
            vector3Scale(&s->up, &tmp1, cosf(angle));
            vector3Scale(&s->right, &tmp2, sinf(angle));
            vector3Add(&tmp1, &tmp2, &up);
            portalAt.scale = gOneVec;

            int portalIndex = -1;
            for (int pi = 0; pi < 2 && portalIndex < 0; ++pi) {
                struct Vector3 look = normal;
                if (pi == 1) vector3Negate(&look, &look);
                quatLook(&look, &up, &portalAt.rotation);
                if (portalSurfaceIsInside(s, &portalAt, pi)) portalIndex = pi;
            }
            if (portalIndex < 0) continue;

            struct Vector2s16 center, outline[PORTAL_LOOP_SIZE], centered[PORTAL_LOOP_SIZE];
            if (!portalSurfaceAdjustPosition(s, &portalAt, &center, outline)) continue;
            for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) vector2s16Sub(&outline[i], &center, &centered[i]);

            for (int step = getenv("STEPMIN") ? atoi(getenv("STEPMIN")) : 1; step <= 100; step += 1) {
                float scale = step / 100.0f;
                int fixedPointScale = (int)(0x10000 * scale);
                struct Vector2s16 loop[PORTAL_LOOP_SIZE];
                for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) {
                    loop[i].x = ((centered[i].x * fixedPointScale) >> 16) + center.x;
                    loop[i].y = ((centered[i].y * fixedPointScale) >> 16) + center.y;
                }

                struct PortalSurface result;
                if (trace) { fprintf(stderr, "cut s%d at %d,%d roll %d scale %.2f loop", si, at.x, at.y, roll, scale); for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) fprintf(stderr, " %d,%d", loop[i].x, loop[i].y); fprintf(stderr, "\n"); }
                ++cuts;
                sTriCount = 0;
                if (!portalSurfacePokeHole(s, loop, &result)) {
                    ++failed;
                    ++refusedAt[(int)(scale * 10 + 0.001f)];
                    if (step == 100) {
                        int outside = 0;
                        for (int i = 0; i < PORTAL_LOOP_SIZE; ++i) outside |= !onSurface(s, &loop[i]);
                        refusedOutside += outside;
                        if (!outside) printf("refused inside: surface %d at %d,%d roll %d\n", si, at.x, at.y, roll);
                    }
                    if (step == 100 && verbose) printf("refused at full size: surface %d %s at %d,%d roll %d\n", si, sNames[si], at.x, at.y, roll);
                    continue;
                }

                double area = 0; int flips = 0;
                for (int t = 0; t < sTriCount; ++t) {
                    double a = tri(result.vertices, sTri[t][0], sTri[t][1], sTri[t][2]) * sign;
                    if (a < -2.0) ++flips;
                    area += a > 0 ? a : 0;
                }
                double expected = wall - loopArea(loop);
                int isMissing = area < expected - 0.01 * wall - 64;

                if (flips) ++flipped;
                if (isMissing) ++missing;
                if ((flips || isMissing) && !surfaceBad) {
                    surfaceBad = 1;
                    ++badSurfaces;
                    printf("surface %d (%s, %dx%d): scale %.2f at %d,%d roll %d: %d flipped, area %.0f of %.0f\n",
                        si, sNames[si], mx.x - mn.x, mx.y - mn.y, scale, at.x, at.y, roll, flips, area, expected);
                } else if (verbose && (flips || isMissing)) {
                    printf("   scale %.2f at %d,%d: %d flipped, area %.0f of %.0f\n", scale, at.x, at.y, flips, area, expected);
                }

                free(result.vertices); free(result.edges); free(result.gfxVertices); free(result.triangles);
            }
        }
    }

    for (int i = 0; i <= 10; ++i) printf("refused at scale %.1f-%.1f: %ld\n", i / 10.0, (i + 1) / 10.0, refusedAt[i]);
    printf("full size refusals with the loop off the surface: %ld\n", refusedOutside);
    printf("\n%ld cuts, %ld refused, %ld with flipped faces, %ld missing area; %d of %d surfaces affected\n",
        cuts, failed, flipped, missing, badSurfaces, SURFACE_COUNT);
    return (flipped || missing) ? 1 : 0;
}
