#include "fizzler_particles.h"

#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_vertex.h"
#include "levels/psp/level_materials.h"
#include "math/mathf.h"
#include "scene/fizzler.h"
#include "util/memory.h"

#include <pspkernel.h>

#include "codegen/assets/materials/static.h"

// The PSP half of the fizzler's particles; see src/scene/n64/fizzler_particles.c.
// One part with one index buffer built once (no vertex cache reloads), in
// the colour vertex format. Malloc'd, since the particles live across frames.

#define IMAGE_WIDTH         16
#define IMAGE_HEIGHT        64

// UVs normalised against the texture (see psp_vertex.h).
#define FIZZLER_U           (IMAGE_WIDTH / (float)IMAGE_WIDTH)
#define FIZZLER_V           (IMAGE_HEIGHT / (float)IMAGE_HEIGHT)

static void fizzlerSpawnParticle(struct Fizzler* fizzler, int particleIndex) {
    int x = (particleIndex & 0x1) ? -fizzler->maxExtent : fizzler->maxExtent;
    int y = randomInRange(-fizzler->maxVerticalExtent, fizzler->maxVerticalExtent);

    int xSize = (particleIndex & 0x1) ? (FIZZLER_PARTICLE_LENGTH_FIXED / 2) : -(FIZZLER_PARTICLE_LENGTH_FIXED / 2);

    struct PspVertexColor* vertex = &((struct PspVertexColor*)fizzler->modelVertices)[particleIndex << 2];

    // The N64's corner order, so the winding matches.
    static const float cornerU[4] = {0.0f, FIZZLER_U, FIZZLER_U, 0.0f};
    static const float cornerV[4] = {0.0f, 0.0f, FIZZLER_V, FIZZLER_V};
    static const int cornerX[4] = {-1, -1, 1, 1};
    static const int cornerY[4] = {-1, 1, 1, -1};

    for (int i = 0; i < 4; ++i) {
        vertex[i].u = cornerU[i];
        vertex[i].v = cornerV[i];
        vertex[i].color = 0xFFFFFFFF;
        vertex[i].x = x + cornerX[i] * xSize;
        vertex[i].y = y + cornerY[i] * (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
        vertex[i].z = 0;
        vertex[i].padding = 0;
    }
}

void fizzlerParticlesInit(struct Fizzler* fizzler) {
    int vertexCount = fizzler->particleCount * 4;
    int indexCount = fizzler->particleCount * 6;

    struct PspVertexColor* vertices = malloc(vertexCount * sizeof(struct PspVertexColor));
    unsigned short* indices = malloc(indexCount * sizeof(unsigned short));
    struct PspModelPart* part = malloc(sizeof(struct PspModelPart));
    struct PspModel* model = malloc(sizeof(struct PspModel));

    fizzler->modelVertices = vertices;

    // Two triangles per quad, as the N64's gSP2Triangles.
    for (int i = 0; i < fizzler->particleCount; ++i) {
        unsigned short base = (unsigned short)(i << 2);
        unsigned short* into = &indices[i * 6];

        into[0] = base + 0; into[1] = base + 1; into[2] = base + 2;
        into[3] = base + 0; into[4] = base + 2; into[5] = base + 3;
    }

    part->vertices = vertices;
    part->indices = indices;
    part->indexCount = (unsigned short)indexCount;
    part->vertexCount = (unsigned short)vertexCount;
    part->vertexFormat = PSP_VERTEX_FORMAT_COLOR;
    part->material = levelMaterial(PORTAL_CLEANSER_INDEX);

    model->parts = part;
    model->partCount = 1;

    fizzler->modelGraphics = model;

    for (int i = 0; i < fizzler->particleCount; ++i) {
        fizzlerSpawnParticle(fizzler, i);

        int offset = fizzler->maxExtent * 2 * (fizzler->particleCount - i) / fizzler->particleCount;

        if (!(i & 0x1)) {
            offset = -offset;
        }

        int maxVertex = (i + 1) << 2;
        for (int currVertex = (i << 2); currVertex < maxVertex; ++currVertex) {
            vertices[currVertex].x += offset;
        }
    }

    sceKernelDcacheWritebackRange(vertices, vertexCount * sizeof(struct PspVertexColor));
    sceKernelDcacheWritebackRange(indices, indexCount * sizeof(unsigned short));
}

void fizzlerParticlesUpdate(struct Fizzler* fizzler) {
    struct PspVertexColor* vertices = fizzler->modelVertices;

    int maxVertex = fizzler->particleCount << 2;

    // Drift direction from bit 2 of the vertex index, as on the N64.
    for (int vertexIndex = 0; vertexIndex < maxVertex; ++vertexIndex) {
        int delta = (vertexIndex & 0x4) ? FIZZLER_UNITS_PER_UPDATE : -FIZZLER_UNITS_PER_UPDATE;
        vertices[vertexIndex].x += delta;
    }

    if ((fizzler->oldestParticleIndex & 0x1) ? vertices[fizzler->oldestParticleIndex << 2].x > fizzler->maxExtent : vertices[fizzler->oldestParticleIndex << 2].x < -fizzler->maxExtent) {
        fizzlerSpawnParticle(fizzler, fizzler->oldestParticleIndex);

        ++fizzler->oldestParticleIndex;

        if (fizzler->oldestParticleIndex == fizzler->particleCount) {
            fizzler->oldestParticleIndex = 0;
        }
    }

    // Written back for the GE.
    sceKernelDcacheWritebackRange(vertices, maxVertex * sizeof(struct PspVertexColor));
}
