#include "graphics/render_scene.h"

#include "levels/levels.h"
#include "sk64/skeletool_defs.h"
#include "util/memory.h"

void renderSceneGenerate(struct RenderScene* renderScene, struct RenderState* renderState) {
    renderScene->renderOrder = stackMalloc(sizeof(short) * renderScene->currentRenderPart);
    renderScene->renderOrderCopy = stackMalloc(sizeof(short) * renderScene->currentRenderPart);

    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        renderScene->renderOrder[i] = i;
    }

    renderSceneSort(renderScene, 0, renderScene->currentRenderPart);

    int prevMaterial = -1;

    gSPDisplayList(renderState->dl++, levelMaterialDefault());
    
    for (int i = 0; i < renderScene->currentRenderPart; ++i) {
        int renderIndex = renderScene->renderOrder[i];

        int materialIndex = renderScene->materials[renderIndex];
    
        if (materialIndex != prevMaterial && materialIndex != -1) {
            if (prevMaterial != -1) {
                gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
            }

            gSPDisplayList(renderState->dl++, levelMaterial(materialIndex));

            prevMaterial = materialIndex;
        }

        struct RenderPart* renderPart = &renderScene->renderParts[renderIndex];

        if (renderPart->matrix) {
            gSPMatrix(renderState->dl++, renderPart->matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
        }

        if (renderPart->armature) {
            gSPSegment(renderState->dl++, MATRIX_TRANSFORM_SEGMENT, renderPart->armature);
        }

        gSPDisplayList(renderState->dl++, renderPart->geometry);

        if (renderPart->matrix) {
            gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
        }
    }

    if (prevMaterial != -1) {
        gSPDisplayList(renderState->dl++, levelMaterialRevert(prevMaterial));
    }
}
