#include "levels.h"

#include "cutscene_runner.h"
#include "physics/collision_scene.h"
#include "player/player.h"
#include "savefile/checkpoint.h"
#include "util/memory.h"

struct LevelDefinition* gCurrentLevel;
void* gLevelSegment;

int gCurrentLevelIndex;

static int sQueuedLevel = NO_QUEUED_LEVEL;
static struct Transform sRelativeTransform = {
    {0.0f, PLAYER_HEAD_HEIGHT, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
};
static struct Vector3 sRelativeVelocity = { 0 };
static int sLoadedFromTransition = 0;

void levelQueueLoad(int index, struct Transform* relativeTransform, struct Vector3* relativeVelocity, int useCheckpoint) {
    if (index == NEXT_LEVEL) {
        sQueuedLevel = gCurrentLevelIndex + 1;

        if (sQueuedLevel == levelCount()) {
            sQueuedLevel = CREDITS_MENU;
        }
    } else {
        sQueuedLevel = index;
    }

    sLoadedFromTransition = 0;

    if (relativeTransform) {
        sRelativeTransform = *relativeTransform;
        sLoadedFromTransition = 1;
    } else {
        transformInitIdentity(&sRelativeTransform);
        sRelativeTransform.position.y = PLAYER_HEAD_HEIGHT;
    }

    if (relativeVelocity) {
        sRelativeVelocity = *relativeVelocity;
        sLoadedFromTransition = 1;
    } else {
        sRelativeVelocity = gZeroVec;
    }

    if (!useCheckpoint) {
        checkpointClear();
    }
}

void levelQueueReload() {
    levelQueueLoad(gCurrentLevelIndex, NULL, NULL, 1 /* useCheckpoint */);
}

int levelGetQueued() {
    return sQueuedLevel;
}

void levelClearQueued() {
    sQueuedLevel = NO_QUEUED_LEVEL;
}


void levelGetStartLocationAndVelocity(struct Location* location, struct Vector3* velocity) {
    struct Location* startLocation = levelGetLocation(gCurrentLevel->startLocation);

    location->roomIndex = startLocation->roomIndex;
    transformConcat(&startLocation->transform, &sRelativeTransform, &location->transform);
    quatMultVector(&startLocation->transform.rotation, &sRelativeVelocity, velocity);
}

int levelLoadedFromTransition() {
    return sLoadedFromTransition;
}

int getChamberIndexFromLevelIndex(int levelIndex, int roomIndex) {
    switch(levelIndex){
        case 0:
            if (roomIndex <= 2)
                return 0;
            else
                return 1;
        case 1:
            if (roomIndex <= 2)
                return 2;
            else
                return 3;
        case 2:
            if (roomIndex <= 2)
                return 4;
            else
                return 5;
        case 3:
            if (roomIndex <= 2)
                return 6;
            else
                return 7;
        case 4:
            return 8;
        case 5:
            return 9;
        case 6:
            return 10;
        case 7:
            if (roomIndex <= 2)
                return 11;
            else
                return 12;
        case 8:
            return 13;
        case 9:
            return 14;
        case 10:
            return 15;
        case 11:
            return 16;
        case 12:
            return 17;
        default:
            return 0;
    }
}

int getLevelIndexFromChamberIndex(int chamberIndex) {
    switch (chamberIndex) {
        case 0:
        case 1:
            return 0;
        case 2:
        case 3:
            return 1;
        case 4:
        case 5:
            return 2;
        case 6:
        case 7:
            return 3;
        case 8:
            return 4;
        case 9:
            return 5;
        case 10:
            return 6;
        case 11:
        case 12:
            return 7;
        case 13:
            return 8;
        case 14:
            return 9;
        case 15:
            return 10;
        case 16:
            return 11;
        case 17:
            return 12;
        default:
            return 0;
    }
}

int levelQuadIndex(struct CollisionObject* pointer) {
    if (pointer < gCollisionScene.quads || pointer >= gCollisionScene.quads + gCollisionScene.quadCount) {
        return -1;
    }

    return pointer - gCollisionScene.quads;
}

struct Location* levelGetLocation(short index) {
    if (index < 0 || index >= gCurrentLevel->locationCount) {
        return NULL;
    }

    return &gCurrentLevel->locations[index];
}
