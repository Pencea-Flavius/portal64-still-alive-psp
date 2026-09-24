#include "fizzler_particles.h"

#include "scene/fizzler.h"
#include "util/memory.h"

// The N64 half of the fizzler's particles; see src/scene/psp/fizzler_particles.c.
//
// A Vtx quad per particle, and one display list that reloads the RSP's vertex
// cache every eight quads because the RSP holds thirty two vertices at a time.

#define IMAGE_WIDTH         16
#define IMAGE_HEIGHT        64

#define GFX_PER_PARTICLE(particleCount) ((particleCount) + (((particleCount) + 7) >> 3) + 1)

static void fizzlerSpawnParticle(struct Fizzler* fizzler, int particleIndex) {
    int x = (particleIndex & 0x1) ? -fizzler->maxExtent : fizzler->maxExtent;
    int y = randomInRange(-fizzler->maxVerticalExtent, fizzler->maxVerticalExtent);

    int xSize = (particleIndex & 0x1) ? (FIZZLER_PARTICLE_LENGTH_FIXED / 2) : -(FIZZLER_PARTICLE_LENGTH_FIXED / 2);

    Vtx* currentVertex = &((Vtx*)fizzler->modelVertices)[particleIndex << 2];

    currentVertex->v.ob[0] = x - xSize;
    currentVertex->v.ob[1] = y - (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = 0;
    currentVertex->v.tc[1] = 0;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;

    ++currentVertex;

    currentVertex->v.ob[0] = x - xSize;
    currentVertex->v.ob[1] = y + (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = IMAGE_WIDTH << 5;
    currentVertex->v.tc[1] = 0;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;

    ++currentVertex;

    currentVertex->v.ob[0] = x + xSize;
    currentVertex->v.ob[1] = y + (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = IMAGE_WIDTH << 5;
    currentVertex->v.tc[1] = IMAGE_HEIGHT << 5;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;

    ++currentVertex;

    currentVertex->v.ob[0] = x + xSize;
    currentVertex->v.ob[1] = y - (FIZZLER_PARTICLE_HEIGHT_FIXED / 2);
    currentVertex->v.ob[2] = 0;

    currentVertex->v.flag = 0;
    currentVertex->v.tc[0] = 0;
    currentVertex->v.tc[1] = IMAGE_HEIGHT << 5;

    currentVertex->v.cn[0] = 255; currentVertex->v.cn[1] = 255; currentVertex->v.cn[2] = 255; currentVertex->v.cn[3] = 255;
}

void fizzlerParticlesInit(struct Fizzler* fizzler) {
    Vtx* vertices = malloc(fizzler->particleCount * 4 * sizeof(Vtx));
    fizzler->modelVertices = vertices;
    fizzler->modelGraphics = malloc(GFX_PER_PARTICLE(fizzler->particleCount) * sizeof(Gfx));

    Gfx* curr = fizzler->modelGraphics;

    for (int currentParticle = 0; currentParticle < fizzler->particleCount; currentParticle += 8) {
        int endParticle = currentParticle + 8;

        if (endParticle > fizzler->particleCount) {
            endParticle = fizzler->particleCount;
        }

        int vertexCount = (endParticle - currentParticle) << 2;

        gSPVertex(curr++, &vertices[currentParticle << 2], vertexCount, 0);

        for (int currentIndex = 0; currentIndex < vertexCount; currentIndex += 4) {
            gSP2Triangles(curr++, 
                currentIndex + 0, currentIndex + 1, currentIndex + 2, 0,
                currentIndex + 0, currentIndex + 2, currentIndex + 3, 0
            );
        }
    }

    gSPEndDisplayList(curr++);

    for (int i = 0; i < fizzler->particleCount; ++i) {
        fizzlerSpawnParticle(fizzler, i);

        int offset = fizzler->maxExtent * 2 * (fizzler->particleCount - i) / fizzler->particleCount;

        if (!(i & 0x1)) {
            offset = -offset;
        }

        int maxVertex = (i + 1) << 2;
        for (int currVertex = (i << 2); currVertex < maxVertex; ++currVertex) {
            vertices[currVertex].v.ob[0] += offset;
        }
    }
}

void fizzlerParticlesUpdate(struct Fizzler* fizzler) {
    Vtx* vertices = fizzler->modelVertices;
    Vtx* currentVertex = vertices;

    int maxVertex = fizzler->particleCount << 2;

    for (int vertexIndex = 0; vertexIndex < maxVertex; ++vertexIndex) {
        int delta = (vertexIndex & 0x4) ? FIZZLER_UNITS_PER_UPDATE : -FIZZLER_UNITS_PER_UPDATE;
        currentVertex->v.ob[0] += delta;
        ++currentVertex;
    }

    if ((fizzler->oldestParticleIndex & 0x1) ? vertices[fizzler->oldestParticleIndex << 2].v.ob[0] > fizzler->maxExtent : vertices[fizzler->oldestParticleIndex << 2].v.ob[0] < -fizzler->maxExtent) {
        fizzlerSpawnParticle(fizzler, fizzler->oldestParticleIndex);

        ++fizzler->oldestParticleIndex;

        if (fizzler->oldestParticleIndex == fizzler->particleCount) {
            fizzler->oldestParticleIndex = 0;
        }
    }

    osWritebackDCache(vertices, sizeof(Vtx) * maxVertex);
}
