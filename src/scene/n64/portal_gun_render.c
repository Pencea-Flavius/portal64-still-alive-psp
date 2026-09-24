#include "scene/portal_gun.h"

#include "effects/portal_trail.h"
#include "levels/material_state.h"
#include "scene/render_plan.h"
#include "sk64/skeletool_armature.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/grav_flare.h"
#include "codegen/assets/models/portal_gun/v_portalgun.h"

// The N64 half of the portal gun's drawing (view model and projectiles); see src/scene/psp/portal_gun_render.c.

#define PORTAL_PROJECTILE_RADIUS    0.15f
#define DISTANCE_FADE_SCALAR        (255.0f / 5.0f)
#define PORTAL_GUN_SCALE            512.0f

void portalBallRender(struct PortalGunProjectile* projectile, struct RenderState* renderState, struct MaterialState* materialState, struct Transform* fromView, int portalIndex) {
    struct Transform transform;

    if (projectile->distance < projectile->maxDistance) {
        vector3AddScaled(
            &projectile->positionDirection.origin,
            &projectile->effectOffset,
            1.0f - projectile->distance / projectile->maxDistance,
            &transform.position
        );
    } else {
        transform.position = projectile->positionDirection.origin;
    }

    transform.rotation = fromView->rotation;
    vector3Scale(&gOneVec, &transform.scale, PORTAL_PROJECTILE_RADIUS);

    Mtx* mtx = renderStateRequestMatrices(renderState, 1);

    transformToMatrixL(&transform, mtx, SCENE_SCALE);

    struct Coloru8* color = &gProjectileColor[portalIndex];

    float alpha = projectile->distance * DISTANCE_FADE_SCALAR;

    if (alpha > 255.0f) {
        alpha = 255.0f;
    }


    if (projectile->distance == 0.0f) {
        materialStateSet(materialState, PORTAL_2_PARTICLE_INDEX, renderState);
    } else {
        materialStateSet(materialState, BRIGHTGLOW_Y_INDEX, renderState);
        gDPSetPrimColor(renderState->dl++, 255, 255, color->r, color->g, color->b, (u8)alpha);
    }

    gSPMatrix(renderState->dl++, mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(renderState->dl++, grav_flare_model_gfx);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}

extern LookAt gLookAt;
extern float getAspect();

void portalGunRenderReal(struct PortalGun* portalGun, struct RenderState* renderState, struct Camera* fromCamera, int lastFiredIndex) {
    struct MaterialState materialState;
    materialStateInit(&materialState, DEFAULT_INDEX);

    for (int i = 0; i < 2; ++i) {
        struct PortalGunProjectile* projectile = &portalGun->projectiles[i];

        portalTrailRender(&projectile->trail, renderState, &materialState, fromCamera, i);

        if (projectile->roomIndex == -1) {
            continue;
        }

        portalBallRender(projectile, renderState, &materialState, &fromCamera->transform, i);
    }

    if (!portalGun->portalGunVisible) {
        return;
    }

    Mtx* matrix = renderStateRequestMatrices(renderState, 2);

    if (!matrix) {
        return;
    }

    u16 perspectiveNormalize;
    guPerspective(&matrix[1], &perspectiveNormalize, portalGun->fov, getAspect(), 0.05f * SCENE_SCALE, 4.0f * SCENE_SCALE, 1.0f);
    gSPMatrix(renderState->dl++, &matrix[1], G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

    gSPLookAt(renderState->dl++, &gLookAt);
    gDPPipeSync(renderState->dl++);
    // set the portal indicator color
    if (lastFiredIndex >= 0 && lastFiredIndex <= 1) {
        struct Coloru8 color = gProjectileColor[lastFiredIndex];
        gDPSetEnvColor(renderState->dl++, color.r, color.g, color.b, 255);
    } else {
        gDPSetEnvColor(renderState->dl++, 128, 128, 128, 255);
    }

    struct Quaternion relativeRotation;
    struct Quaternion inverseCameraRotation;
    quatConjugate(&fromCamera->transform.rotation, &inverseCameraRotation);
    quatMultiply(&inverseCameraRotation, &portalGun->rotation, &relativeRotation);

    quatMultiply(&relativeRotation, &gFlipAroundY, &gGunTransform.rotation);

    LookAt* lookAt = renderStateRequestLookAt(renderState);
    *lookAt = gLookAt;

    struct Vector3 lookDirection;
    quatMultVector(&inverseCameraRotation, &gForward, &lookDirection);
    vector3ToVector3u8(&lookDirection, (struct Vector3u8*)&lookAt->l[0].l.dir);

    quatMultVector(&inverseCameraRotation, &gUp, &lookDirection);
    vector3ToVector3u8(&lookDirection, (struct Vector3u8*)&lookAt->l[1].l.dir);

    gSPLookAt(renderState->dl++, lookAt);

    transformToMatrixL(&gGunTransform, &matrix[0], PORTAL_GUN_SCALE);
    gSPMatrix(renderState->dl++, &matrix[0], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);
    skRenderObject(&portalGun->armature, NULL, renderState);
    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);

    gSPDisplayList(renderState->dl++, static_default);
}

