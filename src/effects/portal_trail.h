#ifndef __PORTAL_TRAIL_H__
#define __PORTAL_TRAIL_H__

#include "graphics/render_types.h"

#include <ultra64.h>

#include "../math/vector3.h"
#include "../math/ray.h"
#include "../math/transform.h"
#include "../graphics/renderstate.h"
#include "../levels/material_state.h"
#include "../graphics/color.h"
#include "../scene/camera.h"

#define PORTAL_PROJECTILE_SPEED     50.0f

struct PortalTrail {
    struct Transform trailTransform;
    RenderMatrix sectionOffset;
    RenderMatrix baseTransform[2];
    struct Vector3 direction;
    short currentBaseTransform;
    float lastDistance;
    float maxDistance;
};

// How far apart the copies of the segment model are down the trail. Both
// render halves step by it.
#define PORTAL_TRAIL_SEGMENT_LENGTH  2.0f

// What both render halves work out first.
struct PortalTrailFade {
    int minDistance;
    int maxDistance;
    struct Coloru8 color;
    unsigned char alpha;
    float startDistance;
};

int portalTrailPrepareFade(struct PortalTrail* trail, struct Camera* fromCamera, int portalIndex, struct PortalTrailFade* out);

void portalTrailInit(struct PortalTrail* trail);
void portalTrailPlay(struct PortalTrail* trail, struct Vector3* from, struct Vector3* to);
void portalTrailUpdate(struct PortalTrail* trail);
// Drawing is split per machine; the build puts one of the two on the path.
#include "portal_trail_render.h"

#endif