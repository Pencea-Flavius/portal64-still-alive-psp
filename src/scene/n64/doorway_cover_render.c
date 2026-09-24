#include "scene/doorway_cover.h"

#include "math/matrix.h"
#include "scene/dynamic_scene.h"

#include "codegen/assets/materials/static.h"

static Vtx cover_vertices[] = {
    {{{-SCENE_SCALE / 2, -SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
    {{{ SCENE_SCALE / 2, -SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
    {{{ SCENE_SCALE / 2,  SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
    {{{-SCENE_SCALE / 2,  SCENE_SCALE / 2, 0}, 0, {0, 0}, {0, 0, 0, 0}}},
};

static Gfx cover_gfx[] = {
    gsSPVertex(cover_vertices, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList()
};

void doorwayCoverRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct DoorwayCover* cover = (struct DoorwayCover*)data;

    // It's difficult to use fog here since portals are their own cameras. Near
    // and far plane distances are relative to each camera, leading to incorrect
    // fade amounts depending on portal placement. CPU-side distance calculation
    // is required regardless to compensate, so simply calculate fade instead.
    float opacity = doorwayCoverOpacity(cover, &renderScene->visibleRooms, &fromView->position);
    if (opacity <= 0.0f) {
        return;
    }

    Mtx* matrix = renderStateRequestMatrices(renderScene->renderState, 1);
    if (!matrix) {
        return;
    }

    Gfx* dl = renderStateAllocateDLChunk(renderScene->renderState, 3);
    Gfx* curr = dl;
    if (!dl) {
        return;
    }

    matrixFromBasisL(
        matrix,
        &cover->definition->position,
        &cover->definition->basis.x,
        &cover->definition->basis.y,
        &cover->definition->basis.z
    );

    struct Coloru8* color = &cover->definition->color;

    gDPSetEnvColor(curr++, color->r, color->g, color->b, opacity * 255.0f);
    gSPDisplayList(curr++, cover_gfx);
    gSPEndDisplayList(curr++);

    renderSceneAdd(
        renderScene,
        dl,
        matrix,
        DOORWAY_COVER_INDEX,
        &cover->definition->position,
        NULL
    );
}
