#include "scene/portal_gun.h"

#include "effects/portal_trail.h"
#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "levels/material_state.h"
#include "levels/psp/level_materials.h"
#include "scene/render_plan.h"
#include "sk64/skeletool_armature.h"

#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/grav_flare.h"
#include "codegen/assets/models/portal_gun/v_portalgun.h"

// The PSP half of the portal gun's drawing -- the view model and the two
// projectiles; see src/scene/n64/portal_gun_render.c.

#define PORTAL_PROJECTILE_RADIUS    0.15f
#define DISTANCE_FADE_SCALAR        (255.0f / 5.0f)
#define PORTAL_GUN_SCALE            512.0f

extern float getAspect();

// The projectile's flare: tinted through a clone in flight, the material
// as is at the muzzle.
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

    RenderMatrices mtx = renderStateTransformToMatrices(renderState, &transform, SCENE_SCALE);

    if (!mtx) {
        return;
    }

    float alpha = projectile->distance * DISTANCE_FADE_SCALAR;

    if (alpha > 255.0f) {
        alpha = 255.0f;
    }

    // The PSP flare model carries its own material, which would override the
    // tint, so draw a one part copy with the material the N64 would have set.
    const struct PspMaterial* useMaterial;

    if (projectile->distance == 0.0f) {
        materialStateSet(materialState, PORTAL_2_PARTICLE_INDEX, renderState);
        useMaterial = levelMaterial(PORTAL_2_PARTICLE_INDEX);
    } else {
        materialStateSet(materialState, BRIGHTGLOW_Y_INDEX, renderState);

        struct PspMaterial* tinted = renderStateRequestMemory(renderState, sizeof(struct PspMaterial));

        if (!tinted) {
            return;
        }

        struct Coloru8 color = gProjectileColor[portalIndex];
        color.a = (unsigned char)alpha;

        *tinted = *levelMaterial(BRIGHTGLOW_Y_INDEX);
        // Only the colour; the material's GU_TFX_BLEND does the glow.
        tinted->primitiveColor = pspRenderColor(&color);
        useMaterial = tinted;
    }

    struct PspModel* flare = renderStateRequestMemory(renderState, sizeof(struct PspModel) + sizeof(struct PspModelPart));

    if (!flare || !useMaterial) {
        return;
    }

    struct PspModelPart* part = (struct PspModelPart*)(flare + 1);
    *flare = grav_flare_model;
    *part = grav_flare_model.parts[0];
    part->material = useMaterial;
    flare->parts = part;
    flare->partCount = 1;

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumMultMatrix((const ScePspFMatrix4*)mtx);

    pspModelDraw(flare);

    sceGumPopMatrix();
}

// The indicator flare's combiner, which the GE cannot run:
//
//     colour = max(0, (T - PRIMITIVE) * T) + T * ENVIRONMENT,   alpha = T
//
// GU_TFX_BLEND with the portal colour as fragment gives the same shape if t
// is each texel's whiteness, so a copy of the texture holding that is built
// once. The alpha is kept.
#define FLARE_PRIMITIVE         (96.0f / 255.0f)
#define FLARE_PIXEL_CAPACITY    2048

static unsigned int sFlareWhiteness[FLARE_PIXEL_CAPACITY] __attribute__((aligned(16)));
static const void* sFlareLevels[PSP_TEXTURE_MAX_LEVELS];
static struct PspTexture sFlareTexture;
static int sFlareBuilt;

// One 8888 texel or palette entry, whitened.
static unsigned int flareWhiteness(unsigned int texel) {
    float t = (texel & 0xFF) * (1.0f / 255.0f);
    float white = (t - FLARE_PRIMITIVE) * t / (1.0f - FLARE_PRIMITIVE);
    unsigned w = white <= 0.0f ? 0 : white >= 1.0f ? 255 : (unsigned)(white * 255.0f);

    return (texel & 0xFF000000) | (w << 16) | (w << 8) | w;
}

static const struct PspTexture* flareWhitenessTexture(const struct PspTexture* source) {
    if (sFlareBuilt) {
        return sFlareBuilt > 0 ? &sFlareTexture : source;
    }

    sFlareBuilt = -1;

    if (!source) {
        return source;
    }

    // Palette textures: only the palette changes.
    if (source->clut) {
        unsigned count = source->format == GU_PSM_T4 ? 16 : 256;

        for (unsigned i = 0; i < count; ++i) {
            sFlareWhiteness[i] = flareWhiteness(source->clut[i]);
        }

        sceKernelDcacheWritebackRange(sFlareWhiteness, count * sizeof(unsigned int));

        sFlareTexture = *source;
        sFlareTexture.clut = sFlareWhiteness;
        sFlareBuilt = 1;

        return &sFlareTexture;
    }

    if (source->format != GU_PSM_8888) {
        return source;
    }

    unsigned used = 0;

    for (unsigned char level = 0; level < source->levelCount; ++level) {
        int width = source->width >> level;
        int height = source->height >> level;
        width = width < 1 ? 1 : width;
        height = height < 1 ? 1 : height;
        // Narrow levels are padded to eight texels, as the renderer binds them.
        unsigned count = (unsigned)((width < 8 ? 8 : width) * height);

        if (used + count > FLARE_PIXEL_CAPACITY) {
            return source;
        }

        const unsigned int* in = source->levels[level];
        unsigned int* out = &sFlareWhiteness[used];

        for (unsigned i = 0; i < count; ++i) {
            out[i] = flareWhiteness(in[i]);
        }

        sFlareLevels[level] = out;
        used += count;
    }

    sceKernelDcacheWritebackRange(sFlareWhiteness, used * sizeof(unsigned int));

    sFlareTexture = *source;
    sFlareTexture.levels = sFlareLevels;
    sFlareBuilt = 1;

    return &sFlareTexture;
}

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

    // The view model has its own near projection so it doesn't clip into the
    // scene; pushed and popped here.
    sceGumMatrixMode(GU_PROJECTION);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    sceGumPerspective(portalGun->fov, getAspect(), 0.05f * SCENE_SCALE, 4.0f * SCENE_SCALE);
    // Camera space, so the view is set aside.
    sceGumMatrixMode(GU_VIEW);
    sceGumPushMatrix();
    sceGumLoadIdentity();
    pspModelViewChanged();

    struct Quaternion relativeRotation;
    struct Quaternion inverseCameraRotation;
    quatConjugate(&fromCamera->transform.rotation, &inverseCameraRotation);

    // The gun's look at: world forward and up in camera space, for the
    // reflections on its metal and glass.
    struct Vector3 lookS;
    struct Vector3 lookT;
    quatMultVector(&inverseCameraRotation, &gForward, &lookS);
    quatMultVector(&inverseCameraRotation, &gUp, &lookT);
    pspSetLookAt(&lookS, &lookT);
    quatMultiply(&inverseCameraRotation, &portalGun->rotation, &relativeRotation);

    quatMultiply(&relativeRotation, &gFlipAroundY, &gGunTransform.rotation);

    RenderMatrices matrix = renderStateTransformToMatrices(renderState, &gGunTransform, PORTAL_GUN_SCALE);

    if (matrix) {
        // The indicator's env colour goes in the primitive colour.
        struct Coloru8 indicator = (lastFiredIndex >= 0 && lastFiredIndex <= 1)
            ? gProjectileColor[lastFiredIndex]
            : (struct Coloru8){128, 128, 128, 255};

        indicator.a = 255;

        sceGumMatrixMode(GU_MODEL);
        sceGumPushMatrix();
        sceGumMultMatrix((const ScePspFMatrix4*)matrix);

        // Drawn under its pose; parts are in bone space.
        const struct PspModel* gun = portalGun->armature.displayList;
        RenderMatrices gunBones = skArmatureBuildTransforms(&portalGun->armature, renderState);

        // Should always be the linked model, but placing a portal sometimes leaves
        // 0x01e35394 here (a stray write, not yet found). Skip the view model for
        // that frame; remove once the write is found.
        if ((unsigned)gun < 0x08800000u) {
            gun = NULL;
        }

        if (gun) {
            unsigned short partCount = gun->partCount;

            unsigned size = sizeof(struct PspModelPart) * partCount
                + sizeof(struct PspMaterial) * partCount;

            struct PspModelPart* parts = renderStateRequestMemory(renderState, size);

            if (parts) {
                struct PspMaterial* materials = (struct PspMaterial*)(parts + partCount);
                // The portal's colour is the fragment; see flareWhitenessTexture().
                unsigned int flareTint = pspRenderColor(&indicator);

                // Only the indicator reads the env colour; tinting every part turned the
                // gun blue.
                for (unsigned short i = 0; i < partCount; ++i) {
                    parts[i] = gun->parts[i];
                    materials[i] = *gun->parts[i].material;

                    if (gun->parts[i].material == &portal_gun_v_portalgun_portal_gun_flare_material) {
                        materials[i].texture = flareWhitenessTexture(materials[i].texture);
                        materials[i].textureFunction = GU_TFX_BLEND;
                        materials[i].envColor = 0xFFFFFFFF;
                        materials[i].primitiveColor = flareTint;
                    }

                    parts[i].material = &materials[i];
                }

                struct PspModel tinted = *gun;
                tinted.parts = parts;

                pspModelSetNoClip(1);
                pspModelDrawSkinned(&tinted, gunBones);
            } else {
                pspModelSetNoClip(1);
                pspModelDrawSkinned(gun, gunBones);
            }

            pspModelSetNoClip(0);
        }

        sceGumPopMatrix();
    }

    sceGumMatrixMode(GU_VIEW);
    sceGumPopMatrix();
    sceGumMatrixMode(GU_PROJECTION);
    sceGumPopMatrix();
    sceGumMatrixMode(GU_MODEL);
    pspModelViewChanged();

    pspMaterialBind(levelMaterialDefault());
}
