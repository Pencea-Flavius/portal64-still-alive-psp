#include "graphics/render_scene.h"

#include "graphics/psp/psp_model_render.h"
#include "math/vector3.h"
#include "levels/levels.h"
#include "levels/material_state.h"
#include "util/memory.h"

#include <pspgu.h>
#include <pspgum.h>

#include "system/psp/psp_profile.h"

// How far moving level geometry (lowered stairs) is pushed behind fixed
// geometry, in metres, clamped to a range of depth units. The floor it
// retracts into is portalable, so the fixed side must win. A distance, since
// a fixed unit count is too much far away.
#define ANIMATED_LEVEL_DEPTH_METRES     0.02f
#define ANIMATED_LEVEL_DEPTH_MIN_UNITS  4
#define ANIMATED_LEVEL_DEPTH_MAX_UNITS  64

// The PSP half of submitting a sorted render scene; see src/graphics/n64/render_scene_generate.c.
// Materials are bound, transforms go on sceGum's stack, and poses go to
// pspModelDrawSkinned(), which walks the bone chains.
void renderSceneGenerate(struct RenderScene* renderScene, struct RenderState* renderState) {
    unsigned long long drawStart = pspProfileDetailNow();

    renderScene->renderOrder = stackMalloc(sizeof(short) * renderScene->currentRenderPart);
    renderScene->renderOrderCopy = stackMalloc(sizeof(short) * renderScene->currentRenderPart);

    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        renderScene->renderOrder[i] = i;
    }

    unsigned long long sortStart = pspProfileDetailNow();
    // Sort models with their own materials by their first one, so props sharing
    // textures draw together. Opaque keys only; translucent keep their order.
    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        const struct PspModel* model = renderScene->renderParts[i].geometry;
        int key = renderScene->sortKeys[i];

        if (!model || !model->partCount || !model->parts[0].material || (key >> 23) >= 0xFF) {
            continue;
        }

        unsigned bucket = (((unsigned)model->parts[0].material >> 4) * 2654435761u) >> 24;
        renderScene->sortKeys[i] = ((int)(bucket % 0xFF) << 23) | (key & 0x7FFFFF);
    }

    renderSceneSort(renderScene, 0, renderScene->currentRenderPart);
    pspProfileDetailAdd(PspProfileSort, sortStart);

    struct MaterialState materialState;
    materialStateInit(&materialState, -1);

    pspModelBeginBatch();

    pspMaterialBind(levelMaterialDefault());

    sceGumMatrixMode(GU_MODEL);

    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        int renderIndex = renderScene->renderOrder[i];

        int materialIndex = renderScene->materials[renderIndex];
        struct RenderPart* renderPart = &renderScene->renderParts[renderIndex];

        // Models whose parts all bind their own material skip the scene's.
        int ownMaterials = renderPart->geometry && renderPart->geometry->partCount;

        for (unsigned short p = 0; ownMaterials && p < renderPart->geometry->partCount; ++p) {
            ownMaterials = renderPart->geometry->parts[p].material != NULL;
        }

        // -1: the part brings its own state, as on the N64.
        if (materialIndex != -1 && !ownMaterials) {
            materialStateSet(&materialState, materialIndex, renderState);
        }

        if (!renderPart->geometry) {
            continue;
        }

        if (renderPart->matrix) {
            sceGumPushMatrix();
            sceGumMultMatrix((const ScePspFMatrix4*)renderPart->matrix);
        }

        // Push moving level parts just behind the floor they lie in.
        if (renderPart->isAnimatedLevel && renderPart->matrix) {
            // The bone's position in metres.
            const float* matrix = (const float*)renderPart->matrix;
            struct Vector3 at = {matrix[12] / SCENE_SCALE, matrix[13] / SCENE_SCALE, matrix[14] / SCENE_SCALE};

            // Negative pushes away (depth is reversed).
            pspModelSetDepthBias(-pspDepthUnitsFor(&at, ANIMATED_LEVEL_DEPTH_METRES, ANIMATED_LEVEL_DEPTH_MIN_UNITS, ANIMATED_LEVEL_DEPTH_MAX_UNITS));
        }

        pspModelDrawSkinned(renderPart->geometry, renderPart->armature);

        if (renderPart->isAnimatedLevel && renderPart->matrix) {
            pspModelSetDepthBias(0);
        }

        // A model with its own materials leaves one bound, so the next chunk must
        // rebind (the N64's display lists revert instead).
        const struct PspModel* model = renderPart->geometry;

        for (int part = 0; part < model->partCount; ++part) {
            if (model->parts[part].material) {
                materialStateInit(&materialState, -1);
                break;
            }
        }

        if (renderPart->matrix) {
            sceGumPopMatrix();
        }
    }

    pspModelEndBatch();

    pspProfileDetailAdd(PspProfileDraw, drawStart);
}
