#ifndef __DECOR_OBJECT_H__
#define __DECOR_OBJECT_H__

#include <ultra64.h>

#include "audio/soundplayer.h"
#include "graphics/render_types.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "physics/collision_object.h"

enum DecorObjectFlags {
    // important objects respawn at their original 
    // location if they escape the level
    DecorObjectFlagsImportant = (1 << 0),
    DecorObjectFlagsMuted = (1 << 1),
};

struct DecorObjectDefinition {
    struct ColliderTypeData colliderType;
    float mass;
    float radius;
    short dynamicModelIndex;
    short materialIndex;
    short materialIndexFizzled;
    short soundClipId;
    short soundFizzleId;
    short flags;
};

struct DecorObject {
    struct CollisionObject collisionObject;
    struct RigidBody rigidBody;
    struct DecorObjectDefinition* definition;
    struct Vector3 originalPosition;
    struct Quaternion originalRotation;
    short originalRoom;
    short dynamicId;
    SoundId playingSound;
    float fizzleTime;
};

struct DecorObject* decorObjectNew(struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectInit(struct DecorObject* object, struct DecorObjectDefinition* definition, struct Transform* at, int room);
void decorObjectCleanup(struct DecorObject* decorObject);
void decorObjectDelete(struct DecorObject* decorObject);

int decorObjectUpdate(struct DecorObject* decorObject);

enum FizzleCheckResult {
    FizzleCheckResultNone,
    FizzleCheckResultStart,
    FizzleCheckResultEnd,
};

struct DynamicRenderDataList;

// Both draw; each machine has a half in src/decor/n64/ and src/decor/psp/.
ModelHandle decorBuildFizzleGfx(ModelHandle gfxToRender, float fizzleTime, struct RenderState* renderState);
void decorObjectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);
enum FizzleCheckResult decorObjectUpdateFizzler(struct CollisionObject* collisionObject, float* fizzleTime);

#endif
