#include "scene/laser.h"

#include <assert.h>

#include "graphics/color.h"
#include "scene/dynamic_scene.h"

#include "codegen/assets/materials/static.h"

static struct Coloru8 laserColor = { 255, 0, 0, 100 };

static void laserBeamBuildVertices(Vtx* vtx, struct LaserBeam* beam, struct Vector3* cameraPosition) {
    // Determine camera-facing up direction for billboard
    struct Vector3 tmp;
    struct Vector3 laserUp;
    vector3Sub(&beam->startPosition.origin, cameraPosition, &tmp);
    vector3Cross(&beam->startPosition.dir, &tmp, &laserUp);
    vector3Normalize(&laserUp, &laserUp);

    // Create vertices for quad
    for (int i = 0; i < 4; ++i, ++vtx) {
        vector3AddScaled(
            (i >> 1) ? &beam->endPosition : &beam->startPosition.origin,
            &laserUp,
            (i & 1) ? -LASER_HALF_WIDTH : LASER_HALF_WIDTH,
            &tmp
        );

        vtx->v.ob[0] = tmp.x * SCENE_SCALE;
        vtx->v.ob[1] = tmp.y * SCENE_SCALE;
        vtx->v.ob[2] = tmp.z * SCENE_SCALE;

        vtx->v.flag = 0;
        vtx->v.tc[0] = 0;
        vtx->v.tc[1] = 0;

        vtx->v.cn[0] = laserColor.r;
        vtx->v.cn[1] = laserColor.g;
        vtx->v.cn[2] = laserColor.b;
        vtx->v.cn[3] = laserColor.a;
    }
}

void laserRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Laser* laser = (struct Laser*)data;

    // Update here (in the render callback) so that it is after game logic.
    // Otherwise the beam positions could be incorrect (parent may have moved).
    // The update function only affects rendering.
    laserUpdate(laser);
    if (laser->beamCount == 0) {
        return;
    }

    // Build quads
    Vtx* vertices = renderStateRequestVertices(renderScene->renderState, laser->beamCount * 4);
    Vtx* curr = vertices;
    for (int i = 0; i < laser->beamCount; ++i, curr += 4) {
        laserBeamBuildVertices(curr, &laser->beams[i], &fromView->position);
    }

    // Render quads
    Gfx* displayList = renderStateAllocateDLChunk(renderScene->renderState, laser->beamCount + 2);
    Gfx* dl = displayList;

    assert(LASER_MAX_BEAMS <= 8);
    gSPVertex(dl++, vertices, 4 * laser->beamCount, 0);

    for (int i = 0; i < laser->beamCount; ++i) {
        short relativeVertex = i << 2;
        gSP2Triangles(
            dl++,
            relativeVertex,
            relativeVertex + 1,
            relativeVertex + 2,
            0,
            relativeVertex + 2,
            relativeVertex + 1,
            relativeVertex + 3,
            0
        );
    }

    gSPEndDisplayList(dl++);

    renderSceneAdd(
        renderScene,
        displayList,
        NULL,
        LASER_INDEX,
        &laser->parent->transform.position,
        NULL
    );
}
