#include "turret_render.h"

#include "decor/decor_object.h"
#include "scene/turret.h"
#include "sk64/skeletool_defs.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_model_list.h"

// The N64 half of the turret's drawing; see src/scene/psp/turret_render.c.

void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = data;

    RenderMatrices matrix;
    RenderMatrices armature;
    int eyeFade;

    if (!turretRenderPrepare(turret, renderState, &matrix, &armature, &eyeFade)) {
        return;
    }

    // Turrets are fizzlable and so their material can differ from the one their
    // display list was generated against. As a result, using a separate
    // material for the eye fade would cause graphical issues when reverting
    // back to the base material.
    //
    // Instead, the turret materials (normal and fizzled) are fade-capable, and
    // the eye is rendered as an attachment so we can control the fade amount
    // before/after it is drawn and not affect the rest of the turret.

    Gfx* dlChunk = renderStateAllocateDLChunk(renderState, 7);
    Gfx* curr = dlChunk;

    Gfx* eyeGfx = curr;
    gDPSetEnvColor(curr++, eyeFade, eyeFade, eyeFade, 0);
    gSPDisplayList(curr++, dynamicAssetModel(PROPS_TURRET_01_EYE_DYNAMIC_MODEL));
    gDPSetEnvColor(curr++, 0, 0, 0, 0);
    gSPEndDisplayList(curr++);

    Gfx* turretGfx = decorBuildFizzleGfx(turret->armature.displayList, turret->fizzleTime, renderState);
    Gfx* finalGfx = curr;
    gSPSegment(curr++, BONE_ATTACHMENT_SEGMENT, osVirtualToPhysical(eyeGfx));
    gSPDisplayList(curr++, turretGfx);
    gSPEndDisplayList(curr++);

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        finalGfx,
        matrix,
        turret->fizzleTime > 0.0f ? TURRET_FIZZLED_INDEX : TURRET_INDEX,
        &turret->rigidBody.transform.position,
        armature,
        turret->rigidBody.flags
    );
}
