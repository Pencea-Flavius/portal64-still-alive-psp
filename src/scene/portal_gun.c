#include "portal_gun.h"

#include "effects/effect_definitions.h"
#include "levels/material_state.h"
#include "physics/collision_scene.h"
#include "physics/collision_cylinder.h"
#include "render_plan.h"
#include "scene.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/portal_gun/v_portalgun.h"

#define PORTAL_GUN_RECOIL_TIME (0.22f)

#define PORTAL_GUN_NEAR_PLANE   0.05f

#define PORTAL_GUN_MOI          0.00395833375f

#define PORTAL_GUN_SCALE        512.0f

struct Quaternion gFlipAroundY = {0.0f, 1.0f, 0.0f, 0.0f};

struct Transform gGunTransform = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
};

void portalGunInit(struct PortalGun* portalGun, struct Transform* at, int isFreshStart) {
    skArmatureInit(&portalGun->armature, &portal_gun_v_portalgun_armature);
    skAnimatorInit(&portalGun->animator, portal_gun_v_portalgun_armature.numberOfBones);
    portalGun->portalGunVisible = 0;
    portalGun->shootAnimationTimer = 0.0;
    portalGun->shootTotalAnimationTimer = 0.0;
    portalGun->fov = DEFAULT_PORTALGUN_FOV;

    portalGun->projectiles[0].roomIndex = -1;
    portalGun->projectiles[1].roomIndex = -1;

    portalTrailInit(&portalGun->projectiles[0].trail);
    portalTrailInit(&portalGun->projectiles[1].trail);

    portalGun->rotation = at->rotation;

    if (isFreshStart) {
        skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_draw_clip, 0.0f, 0);
    } else {
        skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_idle_clip, 0.0f, 0);
    }
}

#define PORTAL_PROJECTILE_RADIUS    0.15f

#define DISTANCE_FADE_SCALAR        (255.0f / 5.0f)

struct Coloru8 gProjectileColor[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};

#define NO_HIT_DISTANCE             20.0f
#define MAX_PROJECTILE_DISTANCE     100.0f

void portalGunUpdatePosition(struct PortalGun* portalGun, struct Player* player) {
    if (player->passedThroughPortal) {
        int portalIndex = player->passedThroughPortal - 1;

        struct Transform* transform = collisionSceneTransformToOtherPortal(portalIndex);

        if (transform) {
            struct Quaternion newRotation;
            quatMultiply(&transform->rotation, &portalGun->rotation, &newRotation);
            portalGun->rotation = newRotation;
        }
    }

    quatLerp(&portalGun->rotation, &player->lookTransform.rotation, 0.45f, &portalGun->rotation);
}

void portalGunUpdate(struct PortalGun* portalGun, struct Player* player) {
    skAnimatorUpdate(&portalGun->animator, portalGun->armature.pose, FIXED_DELTA_TIME);
    portalGunUpdatePosition(portalGun, player);

    if (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun)) {
        portalGun->portalGunVisible = 1;
    } else {
        portalGun->portalGunVisible = 0;
    }

    if (player->flags & PlayerJustShotPortalGun && portalGun->shootAnimationTimer <= 0.0f) {
        portalGun->shootAnimationTimer = PORTAL_GUN_RECOIL_TIME;
        portalGun->shootTotalAnimationTimer = PORTAL_GUN_RECOIL_TIME * 2.0f;
    }

    if (portalGun->shootAnimationTimer >= 0.0f) {
        portalGun->shootAnimationTimer -= FIXED_DELTA_TIME;
        if (portalGun->shootAnimationTimer <= 0.0f){
            portalGun->shootAnimationTimer = 0.0f;
            player->flags &= ~PlayerJustShotPortalGun;
        }
    }
    if (portalGun->shootTotalAnimationTimer >= 0.0f) {
        portalGun->shootTotalAnimationTimer -= FIXED_DELTA_TIME;
        if (portalGun->shootTotalAnimationTimer <= 0.0f){
            portalGun->shootTotalAnimationTimer = 0.0f;
        }
    }

    for (int i = 0; i < 2; ++i) {
        struct PortalGunProjectile* projectile = &portalGun->projectiles[i];

        portalTrailUpdate(&projectile->trail);

        if (projectile->roomIndex == -1) {
            continue;
        }

        struct RaycastHit hit;

        if (collisionSceneRaycast(&gCollisionScene, projectile->roomIndex, &projectile->positionDirection, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME + 0.1f, 0, &hit)) {
            if (!sceneOpenPortalFromHit(
                &gScene,
                &projectile->positionDirection,
                &hit,
                &projectile->playerUp,
                i,
                projectile->roomIndex,
                1,
                0
            )) {
                effectsParticlePlay(&gScene.effects, &gFailPortalSplash[i], &hit.at, &hit.normal, NULL);
            }
            projectile->roomIndex = -1;
        } else {
            projectile->roomIndex = hit.roomIndex;
        }

        vector3AddScaled(
            &projectile->positionDirection.origin,
            &projectile->positionDirection.dir,
            PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME,
            &projectile->positionDirection.origin
        );
        projectile->distance += PORTAL_PROJECTILE_SPEED * FIXED_DELTA_TIME;
    }
}

struct Vector3 gPortalGunExit = {0.0f, 97.0f, 0.0f};

void portalGunFire(struct PortalGun* portalGun, int portalIndex, struct Ray* ray, struct Transform* lookTransform, struct Vector3* playerUp, int roomIndex) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];

    struct RaycastHit hit;

    if (!collisionSceneRaycast(&gCollisionScene, roomIndex, ray, COLLISION_LAYERS_STATIC | COLLISION_LAYERS_BLOCK_PORTAL, 1000000.0f, 0, &hit)) {
        vector3AddScaled(&ray->origin, &ray->dir, NO_HIT_DISTANCE, &hit.at);
        hit.distance = NO_HIT_DISTANCE;
        hit.normal = gZeroVec;
        hit.object = NULL;
        hit.roomIndex = roomIndex;
        hit.throughPortal = NULL;
    }

    projectile->positionDirection = *ray;
    projectile->roomIndex = roomIndex;
    projectile->playerUp = *playerUp;

    projectile->distance = 0.0f;
    projectile->maxDistance = hit.distance;

    skCalculateBonePosition(&portalGun->armature, PORTAL_GUN_V_PORTALGUN_BODY_BONE, &gPortalGunExit, &projectile->effectOffset);

    projectile->effectOffset.x *= -(gScene.camera.fov / (portalGun->fov * PORTAL_GUN_SCALE));
    projectile->effectOffset.y *= (gScene.camera.fov / (portalGun->fov * PORTAL_GUN_SCALE));
    projectile->effectOffset.z *= -1.0f / PORTAL_GUN_SCALE;

    quatMultVector(&portalGun->rotation, &projectile->effectOffset, &projectile->effectOffset);

    struct Vector3 fireFrom;
    vector3Add(&projectile->effectOffset, &ray->origin, &fireFrom);

    portalTrailPlay(&projectile->trail, &fireFrom, &hit.at);
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_fire1_clip, 0.0f, 0);
}


void portalGunFireWorld(struct PortalGun* portalGun, int portalIndex, struct Vector3* from, struct Vector3* to, int roomIndex) {
    struct PortalGunProjectile* projectile = &portalGun->projectiles[portalIndex];
    vector3Sub(to, from, &projectile->positionDirection.dir);
    vector3Normalize(&projectile->positionDirection.dir, &projectile->positionDirection.dir);
    projectile->positionDirection.origin = *from;
    projectile->roomIndex = roomIndex;
    projectile->playerUp = gUp;

    projectile->distance = 0.0f;
    projectile->maxDistance = sqrtf(vector3DistSqrd(from, to));

    projectile->effectOffset = gZeroVec;

    portalTrailPlay(&projectile->trail, from, to);
}

int portalGunIsFiring(struct PortalGun* portalGun){
    if (portalGun->shootTotalAnimationTimer > 0.0f){
        return 1;
    }
    return 0;
}

void portalGunDraw(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_draw_clip, 0.0f, 0);
}

void portalGunFizzle(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_fizzle_clip, 0.0f, 0);
}

void portalGunPickup(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_pickup_clip, 0.0f, 0);
}

void portalGunRelease(struct PortalGun* portalGun) {
    skAnimatorRunClip(&portalGun->animator, &portal_gun_v_portalgun_Armature_release_clip, 0.0f, 0);
}
