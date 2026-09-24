#ifndef __DOORWAY_COVER_H__
#define __DOORWAY_COVER_H__

#include <stdint.h>

#include "graphics/color.h"
#include "levels/level_definition.h"
#include "math/vector3.h"
#include "physics/world.h"
#include "player/player.h"
#include "scene/portal.h"

struct DoorwayCover {
    uint64_t roomFlags;

    struct Doorway* forDoorway;
    struct DoorwayCoverDefinition* definition;
    short dynamicId;
};

// How faded the cover is from where the camera is, which is geometry rather
// than a drawing decision, so both halves of the drawing ask rather than
// working it out.
float doorwayCoverOpacity(struct DoorwayCover* cover, uint64_t* visibleRooms, struct Vector3* viewPosition);

// Drawing is declared in the platform's half, which the build puts on the
// include path. It is a callback the dynamic scene holds, hence the void*.
#include "doorway_cover_render.h"

void doorwayCoverInit(struct DoorwayCover* cover, struct DoorwayCoverDefinition* definition, struct World* world);
int doorwayCoverIsOpaqueFromView(struct DoorwayCover* cover, struct Vector3* viewPosition);

#endif
