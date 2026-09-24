#include "scene/laser.h"

#include "graphics/color.h"
#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "graphics/psp/psp_vertex.h"
#include "graphics/render_scene.h"
#include "levels/levels.h"
#include "scene/dynamic_scene.h"

#include "codegen/assets/materials/static.h"

static struct Coloru8 laserColor = { 255, 0, 0, 100 };

// The N64 writes a vertex cache and a display list of triangle commands into
// the frame; here the same four vertices per beam go into the frame's scratch
// as a PspModel of one part, which is what a runtime model handle is on this
// machine.
static void laserBeamBuildVertices(struct PspVertexColor* vtx, struct LaserBeam* beam, struct Vector3* cameraPosition, unsigned int color) {
    // Determine camera-facing up direction for billboard
    struct Vector3 tmp;
    struct Vector3 laserUp;
    vector3Sub(&beam->startPosition.origin, cameraPosition, &tmp);
    vector3Cross(&beam->startPosition.dir, &tmp, &laserUp);
    vector3Normalize(&laserUp, &laserUp);

    for (int i = 0; i < 4; ++i, ++vtx) {
        vector3AddScaled(
            (i >> 1) ? &beam->endPosition : &beam->startPosition.origin,
            &laserUp,
            (i & 1) ? -LASER_HALF_WIDTH : LASER_HALF_WIDTH,
            &tmp
        );

        vtx->x = tmp.x * SCENE_SCALE;
        vtx->y = tmp.y * SCENE_SCALE;
        vtx->z = tmp.z * SCENE_SCALE;

        vtx->u = 0.0f;
        vtx->v = 0.0f;
        vtx->color = color;
        vtx->padding = 0;
    }
}

void laserRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Laser* laser = (struct Laser*)data;

    // Update here (in the render callback) so that it is after game logic.
    laserUpdate(laser);

    if (laser->beamCount == 0) {
        return;
    }

    struct RenderState* renderState = renderScene->renderState;

    struct PspVertexColor* vertices = renderStateRequestMemory(
        renderState, sizeof(struct PspVertexColor) * laser->beamCount * 4);
    unsigned short* indices = renderStateRequestMemory(
        renderState, sizeof(unsigned short) * laser->beamCount * 6);

    if (!vertices || !indices) {
        return;
    }

    unsigned int color = pspRenderColor(&laserColor);

    for (int i = 0; i < laser->beamCount; ++i) {
        laserBeamBuildVertices(&vertices[i * 4], &laser->beams[i], &fromView->position, color);

        // The same two triangles the N64 spells as one gSP2Triangles.
        unsigned short base = (unsigned short)(i * 4);
        unsigned short* index = &indices[i * 6];

        index[0] = base;
        index[1] = base + 1;
        index[2] = base + 2;
        index[3] = base + 2;
        index[4] = base + 1;
        index[5] = base + 3;
    }

    struct PspModel* model = pspModelBuild(
        renderState,
        vertices, (unsigned short)(laser->beamCount * 4),
        indices, (unsigned short)(laser->beamCount * 6),
        PSP_VERTEX_FORMAT_COLOR,
        levelMaterial(LASER_INDEX)
    );

    if (!model) {
        return;
    }

    renderSceneAdd(
        renderScene,
        model,
        NULL,
        LASER_INDEX,
        &laser->parent->transform.position,
        NULL
    );
}
