#include "doorway_cover.h"

#include "math/matrix.h"
#include "physics/collision_scene.h"
#include "scene/dynamic_scene.h"

#include "codegen/assets/materials/static.h"

static float axisDistance(struct DoorwayCover* cover, struct Vector3* offset) {
    float dist = vector3Dot(&cover->definition->fadeAxis, offset);

    if (cover->definition->fadeAxisDirection == DoorwayCoverFadeAxisDirectionForward) {
        return MAX(0.0f, dist);
    } else {
        return fabsf(dist);
    }
}

static float calculateOpacity(struct DoorwayCover* cover, struct Vector3* offset) {
    float dist = vector3MagSqrd(offset);
    float start = cover->definition->fadeStartDistance;
    float end = cover->definition->fadeEndDistance;

    if (dist <= (start * start)) {
        return 0.0f;
    } else if (dist >= (end * end)) {
        return 1.0f;
    } else {
        return mathfInvLerp(start, end, sqrtf(dist));
    }
}

static float calculateAxisOpacity(struct DoorwayCover* cover, struct Vector3* offset) {
    float dist = axisDistance(cover, offset);
    float start = cover->definition->fadeStartDistance;
    float end = cover->definition->fadeEndDistance;

    if (dist <= start) {
        return 0.0f;
    } else if (dist >= end) {
        return 1.0f;
    } else {
        return mathfInvLerp(start, end, dist);
    }
}

float doorwayCoverOpacity(struct DoorwayCover* cover, uint64_t* visibleRooms, struct Vector3* viewPosition) {
    if ((*visibleRooms & cover->roomFlags) != cover->roomFlags) {
        // One of the rooms isn't visible, but getting here means the cover is.
        // Either the room is explicitly hidden or the fade distance is passed.
        // In both cases the cover should render opaque to hide the void.
        return 1.0f;
    }

    if (cover->definition->fadeEndDistance < 0.0f) {
        return 0.0f;
    }

    struct Vector3 offset;
    vector3Sub(&cover->definition->position, viewPosition, &offset);

    if (cover->definition->fadeAxisDirection != DoorwayCoverFadeAxisDirectionNone) {
        return calculateAxisOpacity(cover, &offset);
    } else {
        return calculateOpacity(cover, &offset);
    }
}

static int doorwayCoverIsCulled(void* data, struct FrustumCullingInformation* frustum) {
    struct DoorwayCover* cover = (struct DoorwayCover*)data;
    return isQuadOutsideFrustum(frustum, &cover->forDoorway->quad);
}

void doorwayCoverInit(struct DoorwayCover* cover, struct DoorwayCoverDefinition* definition, struct World* world) {
    cover->forDoorway = &world->doorways[definition->doorwayIndex];
    cover->roomFlags = ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomA) | ROOM_FLAG_FROM_INDEX(cover->forDoorway->roomB);
    cover->definition = definition;

    float radius = sqrtf(
        (cover->forDoorway->quad.edgeALength * cover->forDoorway->quad.edgeALength) +
        (cover->forDoorway->quad.edgeBLength * cover->forDoorway->quad.edgeBLength)
    ) * 0.5f;
    cover->dynamicId = dynamicSceneAddViewDependent(cover, doorwayCoverRender, &definition->position, radius);
    dynamicSceneSetRoomFlags(cover->dynamicId, cover->roomFlags);
    dynamicSceneSetPreciseCullingCallback(cover->dynamicId, doorwayCoverIsCulled);
}

int doorwayCoverIsOpaqueFromView(struct DoorwayCover* cover, struct Vector3* viewPosition) {
    if (cover->definition->fadeEndDistance < 0.0f) {
        return 0;
    }

    struct Vector3 offset;
    vector3Sub(&cover->definition->position, viewPosition, &offset);

    if (cover->definition->fadeAxisDirection != DoorwayCoverFadeAxisDirectionNone) {
        float threshold = cover->definition->fadeEndDistance;
        return axisDistance(cover, &offset) >= threshold;
    } else {
        float threshold = cover->definition->fadeEndDistance * cover->definition->fadeEndDistance;
        return vector3MagSqrd(&offset) >= threshold;
    }
}
