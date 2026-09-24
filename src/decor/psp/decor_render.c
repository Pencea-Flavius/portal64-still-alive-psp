#include "decor/decor_object.h"

#include "graphics/psp/psp_model.h"
#include "levels/psp/level_materials.h"
#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"

#include <pspgu.h>
#include <pspkernel.h>

// The PSP half of the decor object's drawing and of the fizzle; see src/decor/n64/decor_render.c.

// The fizzle. The N64's fizzled materials compute
//
//     colour = lerp(shade * texel, NOISE, f),   alpha = 1 - f
//
// then blend towards black fog and over the frame, which works out to
//
//     (1 - f)^3 * object + f * (1 - f)^2 * noise + f * frame
//
// The GE has no noise, so: the object at (1 - f)^2 and alpha 1 - f, blended
// normally, then a noise texture at alpha f * (1 - f)^2, added. Cloned into
// the frame, since the material is shared.
#define FIZZLE_NOISE_SIZE 64

static unsigned int sFizzleNoise[FIZZLE_NOISE_SIZE * FIZZLE_NOISE_SIZE] __attribute__((aligned(16)));
static const void* sFizzleNoiseLevels[1] = {sFizzleNoise};
static const struct PspTexture sFizzleNoiseTexture = {
    sFizzleNoiseLevels, 1, FIZZLE_NOISE_SIZE, FIZZLE_NOISE_SIZE, GU_PSM_8888,
};
static unsigned int sFizzleNoiseSeed = 0x1234567;

// New static for every fizzling object, like the RDP's noise.
static void fizzleNoiseRefresh() {
    unsigned int seed = sFizzleNoiseSeed;

    for (int i = 0; i < FIZZLE_NOISE_SIZE * FIZZLE_NOISE_SIZE; ++i) {
        seed = seed * 1664525u + 1013904223u;
        unsigned int value = seed >> 24;
        sFizzleNoise[i] = 0xFF000000 | (value << 16) | (value << 8) | value;
    }

    sFizzleNoiseSeed = seed;
    sceKernelDcacheWritebackRange(sFizzleNoise, sizeof(sFizzleNoise));
}

// The decor object's material for parts without one
// (--psp-default-material-from-scene), set by decorObjectRender().
static const struct PspMaterial* sFizzleSceneMaterial;

ModelHandle decorBuildFizzleGfx(ModelHandle gfxToRender, float fizzleTime, struct RenderState* renderState) {
    if (fizzleTime <= 0.0f || !gfxToRender) {
        return gfxToRender;
    }

    float f = fizzleTime > 1.0f ? 1.0f : fizzleTime;
    float keep = 1.0f - f;

    unsigned int objectAlpha = (unsigned int)(255.0f * keep);
    unsigned int objectByte = (unsigned int)(255.0f * keep * keep);
    unsigned int noiseAlpha = (unsigned int)(255.0f * f * keep * keep);

    unsigned int objectTint = (objectAlpha << 24) | (objectByte << 16) | (objectByte << 8) | objectByte;
    unsigned int noiseTint = (noiseAlpha << 24) | 0x00FFFFFF;

    unsigned short partCount = gfxToRender->partCount;

    // One allocation for the model, both parts and their materials.
    unsigned size = sizeof(struct PspModel)
        + sizeof(struct PspModelPart) * partCount * 2
        + sizeof(struct PspMaterial) * partCount * 2;

    struct PspModel* result = renderStateRequestMemory(renderState, size);

    if (!result) {
        // Out of memory: draw it unfizzled.
        return gfxToRender;
    }

    fizzleNoiseRefresh();

    struct PspModelPart* parts = (struct PspModelPart*)(result + 1);
    struct PspMaterial* materials = (struct PspMaterial*)(parts + partCount * 2);

    for (unsigned short i = 0; i < partCount; ++i) {
        const struct PspMaterial* source = gfxToRender->parts[i].material
            ? gfxToRender->parts[i].material
            : sFizzleSceneMaterial;

        if (!source) {
            return gfxToRender;
        }

        // The object.
        parts[i] = gfxToRender->parts[i];
        materials[i] = *source;
        materials[i].primitiveColor = objectTint;
        materials[i].blend = 1;
        parts[i].material = &materials[i];

        // The static: same triangles, unlit, noise only, no depth write.
        struct PspModelPart* noisePart = &parts[partCount + i];
        struct PspMaterial* noise = &materials[partCount + i];

        *noisePart = gfxToRender->parts[i];
        *noise = *source;
        noise->texture = &sFizzleNoiseTexture;
        noise->textureFunction = GU_TFX_REPLACE;
        noise->textureComponent = GU_TCC_RGB;
        noise->textureFilter = GU_NEAREST;
        noise->wrapS = GU_REPEAT;
        noise->wrapT = GU_REPEAT;
        noise->primitiveColor = noiseTint;
        noise->fragmentSource = PSP_COLOR_SOURCE_PRIMITIVE;
        noise->lighting = 0;
        noise->blend = PSP_BLEND_ADD;
        noise->depthTest = 1;
        noise->depthWrite = 0;
        noise->decal = 0;
        noisePart->material = noise;
    }

    // The rest is the original model, bone table included.
    *result = *gfxToRender;
    result->parts = parts;
    result->partCount = partCount * 2;

    return result;
}

void decorObjectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct DecorObject* object = (struct DecorObject*)data;

    RenderMatrices matrix = renderStateTransformToMatrices(renderState, &object->rigidBody.transform, SCENE_SCALE);

    if (!matrix) {
        return;
    }

    sFizzleSceneMaterial = levelMaterial(object->definition->materialIndex);
    ModelHandle model = decorBuildFizzleGfx(dynamicAssetModel(object->definition->dynamicModelIndex), object->fizzleTime, renderState);
    sFizzleSceneMaterial = NULL;

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        model,
        matrix,
        (object->fizzleTime > 0.0f) ? object->definition->materialIndexFizzled : object->definition->materialIndex,
        &object->rigidBody.transform.position,
        NULL,
        object->rigidBody.flags
    );
}
