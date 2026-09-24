#include "portal_trail_render.h"

#include "effects/portal_trail.h"
#include "graphics/color.h"
#include "levels/material_state.h"
#include "math/mathf.h"
#include "scene/camera.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/portal_gun/ball_trail.h"

// The N64 half of the trail's drawing; see src/effects/psp/portal_trail_render.c.
//
// The fade along the trail is fog, whose near and far are set per draw, and
// the fade in at the tail is the prim colour's alpha.

void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState, struct Camera* fromCamera, int portalIndex) {
    struct PortalTrailFade fade;

    if (!portalTrailPrepareFade(trail, fromCamera, portalIndex, &fade)) {
        return;
    }

    materialStateSet(materialState, PORTAL_TRAIL_INDEX, renderState);

    gSPFogPosition(renderState->dl++, fade.minDistance, fade.maxDistance);
    gDPSetPrimColor(renderState->dl++, 255, 255, fade.color.r, fade.color.g, fade.color.b, fade.alpha);
    gSPMatrix(renderState->dl++, &trail->baseTransform[trail->currentBaseTransform], G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

    float currentDistance = fade.startDistance;
    int hasMore = 1;

    while (hasMore) {
        currentDistance += PORTAL_TRAIL_SEGMENT_LENGTH;

        hasMore = currentDistance < trail->lastDistance && currentDistance < trail->maxDistance;

        if (currentDistance <= 0.0f) {
            continue;
        }

        gSPDisplayList(renderState->dl++, portal_gun_ball_trail_model_gfx);

        if (hasMore) {
            gSPMatrix(renderState->dl++, &trail->sectionOffset, G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_MUL);
        }
    }

    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
}
