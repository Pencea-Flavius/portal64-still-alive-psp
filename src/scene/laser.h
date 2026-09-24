#ifndef __LASER_H__
#define __LASER_H__

#include "math/ray.h"
#include "math/vector3.h"
#include "physics/rigid_body.h"

#define LASER_MAX_BEAMS 4

struct LaserBeam {
    struct Ray startPosition;
    struct Vector3 endPosition;
};

struct Laser {
    struct RigidBody* parent;
    struct Vector3* parentOffset;
    struct Quaternion* parentRotation;
    short dynamicId;

    struct LaserBeam beams[LASER_MAX_BEAMS];
    short beamCount;
};

// How wide a beam is drawn, which both halves of the drawing lay out against.
#define LASER_HALF_WIDTH       0.009375f

// Drawing is declared in the platform's half, which the build puts on the
// include path. It is a callback the dynamic scene holds, hence the void*.
#include "laser_render.h"

void laserInit(struct Laser* laser, struct RigidBody* parent, struct Vector3* offset, struct Quaternion* rotation);
void laserUpdate(struct Laser* laser);
void laserRemove(struct Laser* laser);

#endif
