#include "laser.h"

#include <assert.h>

#include "dynamic_scene.h"
#include "physics/collision_scene.h"


#define LASER_COLLISION_LAYERS (COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_TURRET_SHOTS)

void laserInit(struct Laser* laser, struct RigidBody* parent, struct Vector3* offset, struct Quaternion* rotation) {
    laser->parent = parent;
    laser->parentOffset = offset;
    laser->parentRotation = rotation;
    laser->beamCount = 0;

    // The laser could traverse multiple rooms and pass through portals.
    //
    // To avoid a dynamic object per beam, this single object renders them all,
    // and so efficiently deriving a meaningful culling radius is difficult.
    // Given typical turret gameplay and the low rendering cost (each beam is
    // just a quad), rely solely on room flags.
    laser->dynamicId = dynamicSceneAddViewDependent(
        laser,
        laserRender,
        &parent->transform.position,
        1000000.0f
    );
}

void laserUpdate(struct Laser* laser) {
    struct Ray startPosition = {
        .origin = *laser->parentOffset,
        .dir    = gForward
    };
    quatMultVector(laser->parentRotation, &startPosition.dir, &startPosition.dir);
    rayTransform(&laser->parent->transform, &startPosition, &startPosition);

    // Handle laser and parent in different rooms
    int currentRoom = worldCheckDoorwayCrossings(
        gCollisionScene.world,
        &laser->parent->transform.position,
        &startPosition.origin,
        laser->parent->currentRoom
    );

    uint64_t beamRooms = ROOM_FLAG_FROM_INDEX(currentRoom);
    laser->beamCount = 0;

    for (int i = 0; i < LASER_MAX_BEAMS; ++i) {
        struct LaserBeam* beam = &laser->beams[i];

        struct RaycastHit hit;
        if (!collisionSceneRaycast(
                &gCollisionScene,
                currentRoom,
                &startPosition,
                LASER_COLLISION_LAYERS,
                1000000.0f,
                0,
                &hit)
        ) {
            break;
        }

        beam->startPosition = startPosition;
        beam->endPosition = hit.at;
        beamRooms |= hit.passedRooms;
        ++laser->beamCount;

        int touchingPortals = collisionSceneIsTouchingPortal(&hit.at, &hit.normal);
        if (!touchingPortals) {
            break;
        } else {
            int portalIndex = (touchingPortals & RigidBodyIsTouchingPortal0) ? 0 : 1;

            currentRoom = gCollisionScene.portalRooms[1 - portalIndex];
            startPosition.origin = hit.at;
            rayTransform(collisionSceneTransformToOtherPortal(portalIndex), &startPosition, &startPosition);
        }
    }

    dynamicSceneSetRoomFlags(laser->dynamicId, beamRooms);
}

void laserRemove(struct Laser* laser) {
    if (laser->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(laser->dynamicId);
        laser->dynamicId = INVALID_DYNAMIC_OBJECT;
    }
}
