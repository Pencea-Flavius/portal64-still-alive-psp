#include "fizzler.h"

#include "decor/decor_object_list.h"
#include "graphics/render_scene.h"
#include "math/mathf.h"
#include "physics/collision_scene.h"
#include "scene/dynamic_scene.h"
#include "signals.h"
#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_model_list.h"

#define FRAME_HALF_HEIGHT   1

void fizzlerTrigger(struct CollisionObject* collisionObject, struct CollisionObject* objectEnteringTrigger) {
    struct Fizzler* fizzler = collisionObject->data;
	
    if (objectEnteringTrigger->body) {
        objectEnteringTrigger->body->flags |= RigidBodyFizzled;
    }

    if (fizzler->cubeSignalIndex != -1) {
        int decorType = decorIdForCollisionObject(objectEnteringTrigger);
        if (decorIdIsCube(decorType)) {
            signalsSend(fizzler->cubeSignalIndex);
        }
    }
}

struct Transform gRelativeLeft = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.707106781, 0.0f, 0.707106781},
    {1.0f, 1.0f, 1.0f},
};

struct Transform gRelativeRight = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, -0.707106781, 0.0f, 0.707106781},
    {1.0f, 1.0f, 1.0f},
};

void fizzlerRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Fizzler* fizzler = (struct Fizzler*)data;

    RenderMatrices matrix = renderStateTransformToMatrices(renderState, &fizzler->rigidBody.transform, SCENE_SCALE);

    if (!matrix) {
        return;
    }

    dynamicRenderListAddData(renderList, fizzler->modelGraphics, matrix, PORTAL_CLEANSER_INDEX, &fizzler->rigidBody.transform.position, NULL);

    int halfHeight = fizzler->collisionBox.sideLength.y;
    int rows = (int)(halfHeight / FRAME_HALF_HEIGHT);
    RenderMatrices sideMatrices = renderStateRequestMatrixBlock(renderState, rows * 2);

    if (!sideMatrices) {
        return;
    }

    ModelHandle sideModel = dynamicAssetModel(PROPS_PORTAL_CLEANSER_DYNAMIC_MODEL);
    struct Transform sideTransform;
    int sideY = halfHeight - FRAME_HALF_HEIGHT;

    for (int i = 0; i < rows; ++i, sideY -= FRAME_HALF_HEIGHT * 2) {
        int sideIndex = i * 2;

        gRelativeLeft.position.x = fizzler->collisionBox.sideLength.x;
        gRelativeLeft.position.y = sideY;
        transformConcat(&fizzler->rigidBody.transform, &gRelativeLeft, &sideTransform);
        renderMatrixFromTransform(renderMatricesAt(sideMatrices, sideIndex), &sideTransform, SCENE_SCALE);
        dynamicRenderListAddData(renderList, sideModel, renderMatricesAt(sideMatrices, sideIndex), PORTAL_CLEANSER_WALL_INDEX, &fizzler->rigidBody.transform.position, NULL);

        gRelativeRight.position.x = -fizzler->collisionBox.sideLength.x;
        gRelativeRight.position.y = sideY;
        transformConcat(&fizzler->rigidBody.transform, &gRelativeRight, &sideTransform);
        renderMatrixFromTransform(renderMatricesAt(sideMatrices, sideIndex + 1), &sideTransform, SCENE_SCALE);
        dynamicRenderListAddData(renderList, sideModel, renderMatricesAt(sideMatrices, sideIndex + 1), PORTAL_CLEANSER_WALL_INDEX, &fizzler->rigidBody.transform.position, NULL);
    }
}

void fizzlerInit(struct Fizzler* fizzler, struct Transform* transform, float width, float height, int room, short cubeSignalIndex) {
    fizzler->collisionBox.sideLength.x = width;
    fizzler->collisionBox.sideLength.y = height;
    fizzler->collisionBox.sideLength.z = 0.25f;

    fizzler->colliderType.type = CollisionShapeTypeBox;
    fizzler->colliderType.data = &fizzler->collisionBox;
    fizzler->colliderType.bounce = 0.0f;
    fizzler->colliderType.friction = 0.0f;
    fizzler->colliderType.callbacks = &gCollisionBoxCallbacks;

    collisionObjectInit(&fizzler->collisionObject, &fizzler->colliderType, &fizzler->rigidBody, 1.0f, COLLISION_LAYERS_FIZZLER | COLLISION_LAYERS_BLOCK_PORTAL);
    rigidBodyMarkKinematic(&fizzler->rigidBody);

    fizzler->collisionObject.trigger = fizzlerTrigger;
    fizzler->collisionObject.data = fizzler;
    fizzler->rigidBody.transform = *transform;
    fizzler->rigidBody.currentRoom = room;
    
    fizzler->cubeSignalIndex = cubeSignalIndex;
    
    collisionObjectUpdateBB(&fizzler->collisionObject);
    collisionSceneAddDynamicObject(&fizzler->collisionObject);
    
    fizzler->maxExtent = (int)(maxf(0.0f, width - 0.5f) * SCENE_SCALE);
    fizzler->maxVerticalExtent = (int)(height * SCENE_SCALE);

    fizzler->particleCount = (int)(width * height * FIZZLER_PARTICLES_PER_1x1);

    fizzlerParticlesInit(fizzler);

    fizzler->oldestParticleIndex = 0;
    fizzler->dynamicId = dynamicSceneAdd(fizzler, fizzlerRender, &fizzler->rigidBody.transform.position, sqrtf(width * width + height * height));

    dynamicSceneSetRoomFlags(fizzler->dynamicId, ROOM_FLAG_FROM_INDEX(room));

    dynamicAssetModelPreload(PROPS_PORTAL_CLEANSER_DYNAMIC_MODEL);
}

void fizzlerUpdate(struct Fizzler* fizzler) {
    fizzlerParticlesUpdate(fizzler);
}