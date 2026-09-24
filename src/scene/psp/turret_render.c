#include "turret_render.h"

#include "decor/decor_object.h"
#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_vertex.h"
#include "scene/turret.h"
#include "util/dynamic_asset_loader.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_model_list.h"
#include "codegen/assets/models/props/turret_01.h"

// The PSP half of the turret's drawing; see src/scene/n64/turret_render.c.
// The eye is a separate render list entry with its own tinted material copy
// (as decorBuildFizzleGfx() does), leaving the body's material alone.

// The eye with its vertex colours dimmed by the fade, in the frame's memory.
static ModelHandle turretDimEye(ModelHandle eye, int fade, struct RenderState* renderState) {
    if (!eye || fade <= 0) {
        return eye;
    }

    unsigned keep = fade >= 255 ? 0 : 255 - (unsigned)fade;
    unsigned vertexTotal = 0;

    for (unsigned short i = 0; i < eye->partCount; ++i) {
        vertexTotal += eye->parts[i].vertexCount;
    }

    struct PspModel* result = renderStateRequestMemory(renderState,
        sizeof(struct PspModel) + sizeof(struct PspModelPart) * eye->partCount +
        sizeof(struct PspVertexColor) * vertexTotal);

    if (!result) {
        return eye;
    }

    struct PspModelPart* parts = (struct PspModelPart*)(result + 1);
    struct PspVertexColor* vertices = (struct PspVertexColor*)(parts + eye->partCount);

    for (unsigned short i = 0; i < eye->partCount; ++i) {
        parts[i] = eye->parts[i];

        if (parts[i].vertexFormat != PSP_VERTEX_FORMAT_COLOR) {
            continue;
        }

        const struct PspVertexColor* source = parts[i].vertices;

        for (unsigned short v = 0; v < parts[i].vertexCount; ++v) {
            unsigned int c = source[v].color;
            vertices[v] = source[v];
            vertices[v].color = (c & 0xFF000000) |
                ((((c >> 16) & 0xFF) * keep / 255) << 16) |
                ((((c >> 8) & 0xFF) * keep / 255) << 8) |
                (((c & 0xFF) * keep) / 255);
        }

        parts[i].vertices = vertices;
        vertices += parts[i].vertexCount;
    }

    *result = *eye;
    result->parts = parts;

    return result;
}

void turretRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Turret* turret = data;

    RenderMatrices matrix;
    RenderMatrices armature;
    int eyeFade;

    if (!turretRenderPrepare(turret, renderState, &matrix, &armature, &eyeFade)) {
        return;
    }

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        decorBuildFizzleGfx(turret->armature.displayList, turret->fizzleTime, renderState),
        matrix,
        turret->fizzleTime > 0.0f ? TURRET_FIZZLED_INDEX : TURRET_INDEX,
        &turret->rigidBody.transform.position,
        armature,
        turret->rigidBody.flags
    );

    // The N64's (TEXEL0 - ENVIRONMENT) * SHADE dims the eye by the fade; here
    // its vertices are dimmed instead.
    ModelHandle eye = dynamicAssetModel(PROPS_TURRET_01_EYE_DYNAMIC_MODEL);
    RenderMatrices eyeMatrix = skAttachmentTransform(&turret->armature, PROPS_TURRET_01_ATTACHMENT_EYE_BONE, matrix, armature, renderState);

    if (!eyeMatrix) {
        return;
    }

    ModelHandle dimmedEye = turretDimEye(eye, eyeFade, renderState);

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        dimmedEye,
        eyeMatrix,
        turret->fizzleTime > 0.0f ? TURRET_FIZZLED_INDEX : TURRET_INDEX,
        &turret->rigidBody.transform.position,
        NULL,
        turret->rigidBody.flags
    );
}
