#include "render_scene.h"

#include "levels/levels.h"

#include "util/memory.h"

struct RenderScene* renderSceneNew(struct Transform* cameraTransform, struct RenderState *renderState, u64 visibleRooms) {
    struct RenderScene* result = stackMalloc(sizeof(struct RenderScene));

    struct Vector3 cameraForward;
    quatMultVector(&cameraTransform->rotation, &gForward, &cameraForward);
    vector3Negate(&cameraForward, &cameraForward);
    planeInitWithNormalAndPoint(&result->forwardPlane, &cameraForward, &cameraTransform->position);
    
    result->currentRenderPart = 0;

    int capacity = MAX_RENDER_PART_COUNT;
    result->renderParts = stackMalloc(sizeof(struct RenderPart) * capacity);
    result->sortKeys = stackMalloc(sizeof(int) * capacity);
    result->materials = stackMalloc(sizeof(short) * capacity);

    result->renderOrder = NULL;
    result->renderOrderCopy = NULL;

    result->visibleRooms = visibleRooms;

    result->renderState = renderState;

    return result;
}

void renderSceneFree(struct RenderScene* renderScene) {
    stackMallocFree(renderScene);
}

int renderSceneSortKey(int materialIndex, float distance) {
    int distanceScaled = (int)(distance * SCENE_SCALE);

    // sort transparent surfaces from back to front
    if (materialIndex >= levelMaterialTransparentStart()) {
        return (0xFF << 23) | (0x1000000 - distanceScaled);
    }

    return (materialIndex << 23) | (distanceScaled & 0x7FFFFF);
}

void renderSceneAdd(struct RenderScene* renderScene, ModelHandle geometry, RenderMatrices matrix, int materialIndex, struct Vector3* at, RenderMatrices armature) {
    if (renderScene->currentRenderPart == MAX_RENDER_PART_COUNT) {
        return;
    }

    struct RenderPart* part = &renderScene->renderParts[renderScene->currentRenderPart];
    part->geometry = geometry;
    part->matrix = matrix;
    part->armature = armature;
#ifdef PSP
    part->isAnimatedLevel = 0;
#endif
    renderScene->materials[renderScene->currentRenderPart] = materialIndex;
    renderScene->sortKeys[renderScene->currentRenderPart] = renderSceneSortKey(materialIndex, planePointDistance(&renderScene->forwardPlane, at));

    ++renderScene->currentRenderPart;
}

void renderSceneMarkLastAnimatedLevel(struct RenderScene* renderScene) {
#ifdef PSP
    if (renderScene->currentRenderPart > 0) {
        renderScene->renderParts[renderScene->currentRenderPart - 1].isAnimatedLevel = 1;
    }
#else
    (void)renderScene;
#endif
}

void renderSceneSort(struct RenderScene* renderScene, int min, int max) {
    if (min + 1 >= max) {
        return;
    }

    int middle = (min + max) >> 1;
    renderSceneSort(renderScene, min, middle);
    renderSceneSort(renderScene, middle, max);

    int aHead = min;
    int bHead = middle;
    int output = min;

    while (aHead < middle && bHead < max) {
        int sortDifference = renderScene->sortKeys[renderScene->renderOrder[aHead]] - renderScene->sortKeys[renderScene->renderOrder[bHead]];

        if (sortDifference <= 0) {
            renderScene->renderOrderCopy[output] = renderScene->renderOrder[aHead];
            ++output;
            ++aHead;
        } else {
            renderScene->renderOrderCopy[output] = renderScene->renderOrder[bHead];
            ++output;
            ++bHead;
        }
    }

    while (aHead < middle) {
        renderScene->renderOrderCopy[output] = renderScene->renderOrder[aHead];
        ++output;
        ++aHead;
    }

    while (bHead < max) {
        renderScene->renderOrderCopy[output] = renderScene->renderOrder[bHead];
        ++output;
        ++bHead;
    }

    for (output = min; output < max; ++output) {
        renderScene->renderOrder[output] = renderScene->renderOrderCopy[output];
    }
}
