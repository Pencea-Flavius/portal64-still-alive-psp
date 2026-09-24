#include "signage_render.h"

#include "scene/signage.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/props/signage.h"

// The N64 half of the sign; see src/scene/psp/signage_render.c.
// UVs are s10.5, so a texel is 1 << 5; writes go through K0_TO_K1, which is
// why there is no writeback.

void signageVertexOffsetUV(void* vertices, int uTexels, int vTexels) {
    Vtx* vtx = (Vtx*)K0_TO_K1(vertices);

    for (int i = 0; i < 4; ++i) {
        vtx[i].v.tc[0] += uTexels << 5;
        vtx[i].v.tc[1] += vTexels << 5;
    }
}

void signageVertexSetColor(void* vertices, struct Coloru8* color) {
    Vtx* vtx = (Vtx*)K0_TO_K1(vertices);

    for (int vIndex = 0; vIndex < 4; ++vIndex) {
        vtx[vIndex].v.cn[0] = color->r;
        vtx[vIndex].v.cn[1] = color->g;
        vtx[vIndex].v.cn[2] = color->b;
        vtx[vIndex].v.cn[3] = color->a;
    }
}

void signageVertexSetProgress(void* vertices, short xCoord, int uTexels) {
    Vtx* vtx = (Vtx*)K0_TO_K1(vertices);

    vtx[0].v.ob[0] = xCoord;
    vtx[0].v.tc[0] = uTexels << 5;

    vtx[1].v.ob[0] = xCoord;
    vtx[1].v.tc[0] = uTexels << 5;
}

void signageRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Signage* signage = (struct Signage*)data;

    struct Coloru8 backlightColor;
    struct Coloru8 lcdColor;

    signageRenderPrepare(signage, &backlightColor, &lcdColor);

    RenderMatrices matrix = renderStateTransformToMatrices(renderState, &signage->transform, SCENE_SCALE);

    if (!matrix) {
        return;
    }

    Gfx* model = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = model;

    gDPSetPrimColor(dl++, 255, 255, backlightColor.r, backlightColor.g, backlightColor.b, backlightColor.a);
    gDPSetEnvColor(dl++, lcdColor.r, lcdColor.g, lcdColor.b, lcdColor.a);
    gSPDisplayList(dl++, props_signage_model_gfx);
    gSPEndDisplayList(dl++);

    dynamicRenderListAddData(
        renderList,
        model,
        matrix,
        DEFAULT_INDEX,
        &signage->transform.position,
        NULL
    );
}
