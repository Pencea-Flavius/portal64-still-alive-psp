#include "signage_render.h"

#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_render.h"
#include "graphics/psp/psp_vertex.h"
#include "scene/signage.h"

#include <pspkernel.h>

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/props/signage.h"

// The PSP half of the sign; see src/scene/n64/signage_render.c.
// UVs are normalised, so a shift of N texels is N / width, looked up per
// quad since the digits use different textures (awe_total_copy_1 and _2).

static const struct PspTexture* signageTextureFor(const void* vertices) {
    for (unsigned short i = 0; i < props_signage_model.partCount; ++i) {
        if (props_signage_model.parts[i].vertices == vertices) {
            return props_signage_model.parts[i].material->texture;
        }
    }

    return NULL;
}

void signageVertexOffsetUV(void* vertices, int uTexels, int vTexels) {
    const struct PspTexture* texture = signageTextureFor(vertices);

    if (!texture || !texture->width || !texture->height) {
        return;
    }

    struct PspVertexColor* vtx = vertices;

    float u = uTexels / (float)texture->width;
    float v = vTexels / (float)texture->height;

    for (int i = 0; i < 4; ++i) {
        vtx[i].u += u;
        vtx[i].v += v;
    }

    sceKernelDcacheWritebackRange(vtx, sizeof(struct PspVertexColor) * 4);
}

void signageVertexSetColor(void* vertices, struct Coloru8* color) {
    struct PspVertexColor* vtx = vertices;

    unsigned int packed = pspRenderColor(color);

    for (int vIndex = 0; vIndex < 4; ++vIndex) {
        vtx[vIndex].color = packed;
    }

    sceKernelDcacheWritebackRange(vtx, sizeof(struct PspVertexColor) * 4);
}

void signageVertexSetProgress(void* vertices, short xCoord, int uTexels) {
    const struct PspTexture* texture = signageTextureFor(vertices);

    if (!texture || !texture->width) {
        return;
    }

    struct PspVertexColor* vtx = vertices;

    float u = uTexels / (float)texture->width;

    vtx[0].x = xCoord;
    vtx[0].u = u;

    vtx[1].x = xCoord;
    vtx[1].u = u;

    sceKernelDcacheWritebackRange(vtx, sizeof(struct PspVertexColor) * 2);
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

    // The backlight and LCD colours go through a clone of the materials, placed
    // where each material says (fragmentSource, envSource).
    unsigned short partCount = props_signage_model.partCount;

    unsigned size = sizeof(struct PspModel)
        + sizeof(struct PspModelPart) * partCount
        + sizeof(struct PspMaterial) * partCount;

    struct PspModel* model = renderStateRequestMemory(renderState, size);

    if (!model) {
        return;
    }

    struct PspModelPart* parts = (struct PspModelPart*)(model + 1);
    struct PspMaterial* materials = (struct PspMaterial*)(parts + partCount);

    unsigned int backlight = pspRenderColor(&backlightColor);
    unsigned int lcd = pspRenderColor(&lcdColor);

    for (unsigned short i = 0; i < partCount; ++i) {
        parts[i] = props_signage_model.parts[i];

        materials[i] = *props_signage_model.parts[i].material;
        materials[i].primitiveColor = backlight;

        if (materials[i].envSource == PSP_COLOR_SOURCE_PRIMITIVE) {
            materials[i].envColor = backlight;
        } else if (materials[i].envSource == PSP_COLOR_SOURCE_ENVIRONMENT) {
            materials[i].envColor = lcd;
        }

        // Parts that ignore shade carry their colour in the vertices, so write it
        // there (in the model's own vertices, like signageVertexSetColor()).
        if (materials[i].fragmentSource != PSP_COLOR_SOURCE_SHADE &&
            parts[i].vertexFormat == PSP_VERTEX_FORMAT_COLOR) {
            unsigned int fragment = materials[i].fragmentSource == PSP_COLOR_SOURCE_ENVIRONMENT ? lcd : backlight;
            struct PspVertexColor* vtx = (struct PspVertexColor*)parts[i].vertices;

            if (parts[i].vertexCount && vtx[0].color != fragment) {
                for (unsigned short v = 0; v < parts[i].vertexCount; ++v) {
                    vtx[v].color = fragment;
                }

                sceKernelDcacheWritebackRange(vtx, sizeof(struct PspVertexColor) * parts[i].vertexCount);
            }
        }

        parts[i].material = &materials[i];
    }

    *model = props_signage_model;
    model->parts = parts;

    dynamicRenderListAddData(
        renderList,
        model,
        matrix,
        DEFAULT_INDEX,
        &signage->transform.position,
        NULL
    );
}
