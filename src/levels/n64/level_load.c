#include "levels/levels.h"

#include "physics/collision_scene.h"
#include "levels/cutscene_runner.h"
#include "system/cartridge.h"
#include "util/memory.h"

#include "codegen/assets/test_chambers/level_list.h"

// The N64 half of loading a level (src/levels/psp/...): copy the level's ROM
// segment into RAM and shift every pointer in it by how far it moved.

#define ADJUST_POINTER_POS(ptr, offset) (void*)((ptr) ? (char*)(ptr) + (offset) : 0)

static struct LevelDefinition* levelFixPointers(struct LevelDefinition* from, int pointerOffset) {
    struct LevelDefinition* result = ADJUST_POINTER_POS(from, pointerOffset);

    result->collisionQuads = ADJUST_POINTER_POS(result->collisionQuads, pointerOffset);

    for (int i = 0; i < result->collisionQuadCount; ++i) {
        result->collisionQuads[i].collider = ADJUST_POINTER_POS(result->collisionQuads[i].collider, pointerOffset);
        result->collisionQuads[i].collider->data = ADJUST_POINTER_POS(result->collisionQuads[i].collider->data, pointerOffset);
        result->collisionQuads[i].body = ADJUST_POINTER_POS(result->collisionQuads[i].body, pointerOffset);
        result->collisionQuads[i].data = ADJUST_POINTER_POS(result->collisionQuads[i].data, pointerOffset);
    }

    result->namedColliderIndices = ADJUST_POINTER_POS(result->namedColliderIndices, pointerOffset);

    result->staticContent = ADJUST_POINTER_POS(result->staticContent, pointerOffset);
    result->staticBoundingBoxes = ADJUST_POINTER_POS(result->staticBoundingBoxes, pointerOffset);
    result->roomStaticMapping = ADJUST_POINTER_POS(result->roomStaticMapping, pointerOffset);
    result->signalToStaticRanges = ADJUST_POINTER_POS(result->signalToStaticRanges, pointerOffset);
    result->signalToStaticIndices = ADJUST_POINTER_POS(result->signalToStaticIndices, pointerOffset);
    result->portalSurfaces = ADJUST_POINTER_POS(result->portalSurfaces, pointerOffset);

    for (int i = 0; i < result->portalSurfaceCount; ++i) {
        result->portalSurfaces[i].vertices = ADJUST_POINTER_POS(result->portalSurfaces[i].vertices, pointerOffset);
        result->portalSurfaces[i].edges = ADJUST_POINTER_POS(result->portalSurfaces[i].edges, pointerOffset);
        result->portalSurfaces[i].gfxVertices = ADJUST_POINTER_POS(result->portalSurfaces[i].gfxVertices, pointerOffset);
    }

    result->portalSurfaceMappingRange = ADJUST_POINTER_POS(result->portalSurfaceMappingRange, pointerOffset);
    result->portalSurfaceDynamicMappingRange = ADJUST_POINTER_POS(result->portalSurfaceDynamicMappingRange, pointerOffset);
    result->portalSurfaceMappingIndices = ADJUST_POINTER_POS(result->portalSurfaceMappingIndices, pointerOffset);
    result->triggers = ADJUST_POINTER_POS(result->triggers, pointerOffset);

    for (int i = 0; i < result->triggerCount; ++i) {
        result->triggers[i].triggers = ADJUST_POINTER_POS(result->triggers[i].triggers, pointerOffset);
    }

    result->cutscenes = ADJUST_POINTER_POS(result->cutscenes, pointerOffset);

    for (int i = 0; i < result->cutsceneCount; ++i) {
        result->cutscenes[i].steps = ADJUST_POINTER_POS(result->cutscenes[i].steps, pointerOffset);
    }

    result->locations = ADJUST_POINTER_POS(result->locations, pointerOffset);
    result->roomBvhList = ADJUST_POINTER_POS(result->roomBvhList, pointerOffset);
    result->world.rooms = ADJUST_POINTER_POS(result->world.rooms, pointerOffset);
    result->world.doorways = ADJUST_POINTER_POS(result->world.doorways, pointerOffset);

    for (int i = 0; i < result->world.roomCount; ++i) {
        result->world.rooms[i].quadIndices = ADJUST_POINTER_POS(result->world.rooms[i].quadIndices, pointerOffset);
        result->world.rooms[i].cellContents = ADJUST_POINTER_POS(result->world.rooms[i].cellContents, pointerOffset);
        result->world.rooms[i].doorwayIndices = ADJUST_POINTER_POS(result->world.rooms[i].doorwayIndices, pointerOffset);

        result->roomBvhList[i].boxIndex = ADJUST_POINTER_POS(result->roomBvhList[i].boxIndex, pointerOffset);
        result->roomBvhList[i].animatedBoxes = ADJUST_POINTER_POS(result->roomBvhList[i].animatedBoxes, pointerOffset);
    }

    result->doors = ADJUST_POINTER_POS(result->doors, pointerOffset);
    result->doorwayCovers = ADJUST_POINTER_POS(result->doorwayCovers, pointerOffset);
    result->buttons = ADJUST_POINTER_POS(result->buttons, pointerOffset);
    result->signalOperators = ADJUST_POINTER_POS(result->signalOperators, pointerOffset);
    result->decor = ADJUST_POINTER_POS(result->decor, pointerOffset);
    result->fizzlers = ADJUST_POINTER_POS(result->fizzlers, pointerOffset);
    result->elevators = ADJUST_POINTER_POS(result->elevators, pointerOffset);
    result->pedestals = ADJUST_POINTER_POS(result->pedestals, pointerOffset);
    result->signage = ADJUST_POINTER_POS(result->signage, pointerOffset);
    result->boxDroppers = ADJUST_POINTER_POS(result->boxDroppers, pointerOffset);
    result->switches = ADJUST_POINTER_POS(result->switches, pointerOffset);
    result->dynamicBoxes = ADJUST_POINTER_POS(result->dynamicBoxes, pointerOffset);
    result->ballLaunchers = ADJUST_POINTER_POS(result->ballLaunchers, pointerOffset);
    result->ballCatchers = ADJUST_POINTER_POS(result->ballCatchers, pointerOffset);
    result->clocks = ADJUST_POINTER_POS(result->clocks, pointerOffset);
    result->securityCameras = ADJUST_POINTER_POS(result->securityCameras, pointerOffset);
    result->turrets = ADJUST_POINTER_POS(result->turrets, pointerOffset);
    result->incinerators = ADJUST_POINTER_POS(result->incinerators, pointerOffset);

    result->animations = ADJUST_POINTER_POS(result->animations, pointerOffset);

    for (int i = 0; i < result->animationInfoCount; ++i) {
        result->animations[i].clips = ADJUST_POINTER_POS(result->animations[i].clips, pointerOffset);
        result->animations[i].armature.boneParentIndex = ADJUST_POINTER_POS(result->animations[i].armature.boneParentIndex, pointerOffset);
        result->animations[i].armature.pose = ADJUST_POINTER_POS(result->animations[i].armature.pose, pointerOffset);
    }

    return result;
}

void levelLoad(int index) {
    if (index < 0 || index >= LEVEL_COUNT) {
        return;
    }

    struct LevelMetadata* metadata = &gLevels[index];

    void* memory = malloc(metadata->segmentRomEnd - metadata->segmentRomStart);
    romCopy(metadata->segmentRomStart, memory, metadata->segmentRomEnd - metadata->segmentRomStart);

    gLevelSegment = memory;

    gCurrentLevel = levelFixPointers(metadata->levelDefinition, (char*)memory - metadata->segmentStart);
    gCurrentLevelIndex = index;

    collisionSceneInit(&gCollisionScene, gCurrentLevel->collisionQuads, gCurrentLevel->collisionQuadCount, &gCurrentLevel->world);
    cutsceneRunnerReset();
}

// The generated list defines gLevels, so it is included only here.
int levelCount() {
    return LEVEL_COUNT;
}
