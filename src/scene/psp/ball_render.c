#include "ball_render.h"

#include "scene/ball.h"

#include <pspgu.h>

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/fleck_ash2.h"
#include "codegen/assets/models/grav_flare.h"

// The flare is a camera facing quad the asset pipeline produced, so both
// machines draw the same model and differ only in how they spell it.

void ballRender(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct Ball* ball = (struct Ball*)data;

    struct Transform transform;
    transform.position = ball->rigidBody.transform.position;
    transform.rotation = fromView->rotation;
    vector3Scale(&gOneVec, &transform.scale, BALL_RADIUS);

    RenderMatrices mtx = renderStateTransformToMatrices(renderScene->renderState, &transform, SCENE_SCALE);

    if (!mtx) {
        return;
    }

    // The fade is the material's primitive alpha; one part, so copy the model,
    // its part and the material into the frame's scratch.
    struct PspModel* model = renderStateRequestMemory(
        renderScene->renderState,
        sizeof(struct PspModel) + sizeof(struct PspModelPart) + sizeof(struct PspMaterial)
    );

    if (!model) {
        return;
    }

    struct PspModelPart* part = (struct PspModelPart*)(model + 1);
    struct PspMaterial* material = (struct PspMaterial*)(part + 1);

    *model = grav_flare_model;
    *part = grav_flare_model.parts[0];
    *material = *part->material;
    // Generated as REPLACE, which would ignore the fade.
    material->textureFunction = GU_TFX_MODULATE;
    material->primitiveColor = ((unsigned int)(ballOpacity(ball) * 255.0f) << 24) | 0x00FFFFFF;
    part->material = material;
    model->parts = part;

    renderSceneAdd(renderScene, model, mtx, GRAV_FLARE_INDEX, &ball->rigidBody.transform.position, NULL);
}

void ballBurnRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct BallBurnMark* burn = (struct BallBurnMark*)data;

    dynamicRenderListAddData(
        renderList,
        &fleck_ash2_model,
        &burn->matrix,
        FLECK_ASH2_INDEX,
        &burn->at,
        NULL
    );
}
