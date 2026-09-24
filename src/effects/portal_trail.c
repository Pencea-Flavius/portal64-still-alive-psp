#include "portal_trail.h"

#include "graphics/color.h"
#include "math/vector2.h"
#include "math/mathf.h"
#include "util/frame_time.h"

#include "codegen/assets/models/portal_gun/ball_trail.h"
#include "codegen/assets/materials/static.h"

#define TRAIL_LENGTH    8.0f
#define FADE_IN_LENGTH  4.0f
#define SEGMENT_LENGTH  PORTAL_TRAIL_SEGMENT_LENGTH
#define SEGMENT_ROTATION    (152 * M_PI / 180.0f)

struct Transform gTrailSectionOffset = {
    {0.0f, 0.0f, -SEGMENT_LENGTH},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
};

void portalTrailInit(struct PortalTrail* trail) {
    quatAxisAngle(&gForward, SEGMENT_ROTATION, &gTrailSectionOffset.rotation);
    renderMatrixFromTransform(&trail->sectionOffset, &gTrailSectionOffset, SCENE_SCALE);
    renderMatrixIdentity(&trail->baseTransform[0]);
    renderMatrixIdentity(&trail->baseTransform[1]);

    trail->currentBaseTransform = 0;
    trail->lastDistance = 0.0f;
    trail->maxDistance = -TRAIL_LENGTH;
}

void portalTrailUpdateBaseTransform(struct PortalTrail* trail) {
    trail->currentBaseTransform ^= 1;
    renderMatrixFromTransform(&trail->baseTransform[trail->currentBaseTransform], &trail->trailTransform, SCENE_SCALE);
    renderMatrixFlush(&trail->baseTransform[trail->currentBaseTransform]);
}

void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to) {
    trail->trailTransform.position = *from;
    struct Vector3 dir;
    vector3Sub(to, from, &dir);
    struct Vector3 randomUp;
    randomUp.x = randomInRangef(-1.0f, 1.0f);
    randomUp.y = randomInRangef(-1.0f, 1.0f);
    randomUp.z = randomInRangef(-1.0f, 1.0f);
    quatLook(&dir, &randomUp, &trail->trailTransform.rotation);
    vector3Normalize(&dir, &trail->direction);
    trail->trailTransform.scale = gOneVec;

    portalTrailUpdateBaseTransform(trail);

    trail->lastDistance = 0.0f;
    trail->maxDistance = vector3Dot(&dir, &trail->direction);

    if (trail->maxDistance < SEGMENT_LENGTH) {
        trail->maxDistance = -TRAIL_LENGTH;
    }
}

void portalTrailUpdate(struct PortalTrail* trail) {
    if (trail->lastDistance >= trail->maxDistance + TRAIL_LENGTH) {
        return;
    }

    trail->lastDistance += FIXED_DELTA_TIME * PORTAL_PROJECTILE_SPEED;

    if (trail->lastDistance > TRAIL_LENGTH + SEGMENT_LENGTH) {
        trail->lastDistance -= SEGMENT_LENGTH;
        trail->maxDistance -= SEGMENT_LENGTH;
        
        struct Transform tmp;
        transformConcat(&trail->trailTransform, &gTrailSectionOffset, &tmp);
        trail->trailTransform = tmp;
        portalTrailUpdateBaseTransform(trail);
    }
}

struct Coloru8 gTrailColor[] = {
    {200, 100, 50, 255},
    {50, 70, 200, 255},
};


// The trail's fog distances, colour and fade in, for both render halves.
// Returns 0 when the trail is done and nothing should be drawn.
int portalTrailPrepareFade(struct PortalTrail* trail, struct Camera* fromCamera, int portalIndex, struct PortalTrailFade* out) {
    if (trail->lastDistance >= trail->maxDistance + TRAIL_LENGTH) {
        return 0;
    }

    struct Ray cameraRay;
    cameraRay.origin = fromCamera->transform.position;
    quatMultVector(&fromCamera->transform.rotation, &gForward, &cameraRay.dir);
    vector3Negate(&cameraRay.dir, &cameraRay.dir);

    struct Vector3 pointAlongTrail;
    vector3AddScaled(&trail->trailTransform.position, &trail->direction, trail->lastDistance - TRAIL_LENGTH, &pointAlongTrail);
    int minDistance = fogIntValue(cameraClipDistance(fromCamera, rayDetermineDistance(&cameraRay, &pointAlongTrail)));

    vector3AddScaled(&trail->trailTransform.position, &trail->direction, trail->lastDistance, &pointAlongTrail);
    int maxDistance = fogIntValue(cameraClipDistance(fromCamera, rayDetermineDistance(&cameraRay, &pointAlongTrail)));

    if (maxDistance <= minDistance) {
        maxDistance = minDistance + 1;

        if (maxDistance > 1000) {
            maxDistance = 1000;
            minDistance = 999;
        }
    }

    float currentDistance = trail->lastDistance - TRAIL_LENGTH;

    int alpha = 0;

    if (currentDistance < -(TRAIL_LENGTH - FADE_IN_LENGTH)) {
        alpha = (int)(((TRAIL_LENGTH - FADE_IN_LENGTH) + currentDistance) * (-255.0f / TRAIL_LENGTH));

        if (alpha < 0) {
            alpha = 0;
        }

        if (alpha > 255) {
            alpha = 255;
        }
    }

    out->minDistance = minDistance;
    out->maxDistance = maxDistance;
    out->color = gTrailColor[portalIndex];
    out->alpha = (unsigned char)alpha;
    out->startDistance = currentDistance;

    return 1;
}
