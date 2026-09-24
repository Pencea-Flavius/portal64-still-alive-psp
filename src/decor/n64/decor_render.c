#include "decor/decor_object.h"

#include "scene/dynamic_scene.h"
#include "util/dynamic_asset_loader.h"

ModelHandle decorBuildFizzleGfx(ModelHandle gfxToRender, float fizzleTime, struct RenderState* renderState) {
    if (fizzleTime <= 0.0f) {
        return gfxToRender;
    }

    Gfx* result = renderStateAllocateDLChunk(renderState, 3);

    Gfx* curr = result;

    int fizzleTimeAsInt = (int)(255.0f * fizzleTime);

    if (fizzleTimeAsInt > 255) {
        fizzleTimeAsInt = 255;
    }

    gDPSetPrimColor(curr++, 255, 255, fizzleTimeAsInt, fizzleTimeAsInt, fizzleTimeAsInt, 255 - fizzleTimeAsInt);
    gSPDisplayList(curr++, gfxToRender);
    gSPEndDisplayList(curr++);

    return result;
}

void decorObjectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct DecorObject* object = (struct DecorObject*)data;

    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

    if (!matrix) {
        return;
    }

    transformToMatrixL(&object->rigidBody.transform, matrix, SCENE_SCALE);

    dynamicRenderListAddDataTouchingPortal(
        renderList, 
        decorBuildFizzleGfx(dynamicAssetModel(object->definition->dynamicModelIndex), object->fizzleTime, renderState), 
        matrix, 
        (object->fizzleTime > 0.0f) ? object->definition->materialIndexFizzled : object->definition->materialIndex, 
        &object->rigidBody.transform.position, 
        NULL,
        object->rigidBody.flags
    );
}
