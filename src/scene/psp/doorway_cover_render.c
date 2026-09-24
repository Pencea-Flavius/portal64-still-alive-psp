#include "scene/doorway_cover.h"

#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "graphics/psp/psp_vertex.h"
#include "graphics/render_scene.h"
#include "levels/levels.h"
#include "math/matrix.h"
#include "scene/dynamic_scene.h"

#include <pspgum.h>

#include "codegen/assets/materials/static.h"

// The N64 keeps one static quad and one display list and tints them with an
// env colour before submitting. Here the tint is per vertex, so the quad is
// built into the frame's scratch each time with the colour already in it --
// four vertices is cheaper than a second material would be.
static const short coverCorners[4][2] = {
    {-SCENE_SCALE / 2, -SCENE_SCALE / 2},
    { SCENE_SCALE / 2, -SCENE_SCALE / 2},
    { SCENE_SCALE / 2,  SCENE_SCALE / 2},
    {-SCENE_SCALE / 2,  SCENE_SCALE / 2},
};

// The same two triangles the N64 spells as one gSP2Triangles.
static const unsigned short coverIndices[6] = {0, 1, 2, 0, 2, 3};

void doorwayCoverRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct DoorwayCover* cover = (struct DoorwayCover*)data;

    // It's difficult to use fog here since portals are their own cameras, so
    // the fade is worked out on the CPU; doorwayCoverOpacity() is shared.
    float opacity = doorwayCoverOpacity(cover, &renderScene->visibleRooms, &fromView->position);

    if (opacity <= 0.0f) {
        return;
    }

    struct RenderState* renderState = renderScene->renderState;

    struct PspVertexColor* vertices = renderStateRequestMemory(
        renderState, sizeof(struct PspVertexColor) * 4);
    unsigned short* indices = renderStateRequestMemory(renderState, sizeof(coverIndices));

    if (!vertices || !indices) {
        return;
    }

    float basis[4][4];

    matrixFromBasis(
        basis,
        &cover->definition->position,
        &cover->definition->basis.x,
        &cover->definition->basis.y,
        &cover->definition->basis.z
    );

    RenderMatrices matrix = renderStateMatrixFromFloat(renderState, basis);

    if (!matrix) {
        return;
    }

    struct Coloru8 tint = cover->definition->color;
    tint.a = (unsigned char)(opacity * 255.0f);

    unsigned int color = pspRenderColor(&tint);

    for (int i = 0; i < 4; ++i) {
        vertices[i].x = coverCorners[i][0];
        vertices[i].y = coverCorners[i][1];
        vertices[i].z = 0;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
        vertices[i].color = color;
        vertices[i].padding = 0;
    }

    for (unsigned i = 0; i < sizeof(coverIndices) / sizeof(*coverIndices); ++i) {
        indices[i] = coverIndices[i];
    }

    // The material takes its colour from the environment (set per draw on the
    // N64); here the colour and fade are in the vertices, so draw with a copy
    // that reads them instead of replacing them with the material's white.
    static struct PspMaterial sCoverMaterial;

    if (!sCoverMaterial.name) {
        sCoverMaterial = *levelMaterial(DOORWAY_COVER_INDEX);
        sCoverMaterial.fragmentSource = PSP_COLOR_SOURCE_SHADE;
    }

    struct PspModel* model = pspModelBuild(
        renderState,
        vertices, 4,
        indices, sizeof(coverIndices) / sizeof(*coverIndices),
        PSP_VERTEX_FORMAT_COLOR,
        &sCoverMaterial
    );

    if (!model) {
        return;
    }

    renderSceneAdd(
        renderScene,
        model,
        matrix,
        DOORWAY_COVER_INDEX,
        &cover->definition->position,
        NULL
    );
}
