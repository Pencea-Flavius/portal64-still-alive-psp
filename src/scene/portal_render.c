#include "portal_render.h"

#include "render_plan.h"

struct Quaternion gVerticalFlip = {0.0f, 1.0f, 0.0f, 0.0f};

void portalDetermineTransform(struct Portal* portal, float portalTransform[4][4]) {
    struct Transform finalTransform;
    finalTransform = portal->rigidBody.transform;

    if (portal->flags & PortalFlagsOddParity) {
        quatMultiply(&portal->rigidBody.transform.rotation, &gVerticalFlip, &finalTransform.rotation);
    }
    
    vector3Scale(&gOneVec, &finalTransform.scale, portal->scale);

    transformToMatrix(&finalTransform, portalTransform, SCENE_SCALE);
}

