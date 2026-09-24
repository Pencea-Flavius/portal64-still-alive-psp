#include "ball_render.h"

#include "scene/ball.h"

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

    renderSceneAdd(renderScene, &grav_flare_model, mtx, GRAV_FLARE_INDEX, &ball->rigidBody.transform.position, NULL);
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
