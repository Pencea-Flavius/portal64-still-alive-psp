#ifndef __LEVELS_H__
#define __LEVELS_H__

#include "level_definition.h"

#define CREDITS_MENU    -5
#define INTRO_MENU      -4
#define MAIN_MENU       -3
#define NO_QUEUED_LEVEL -2
#define NEXT_LEVEL      -1

extern struct LevelDefinition* gCurrentLevel;
extern int gCurrentLevelIndex;

// Where the level's data was loaded. The loader writes it and the renderer
// reads it -- on the N64 to point a segment at it -- so it belongs with the
// level rather than with the frame, which is where it used to be declared.
extern void* gLevelSegment;

void levelQueueLoad(int index, struct Transform* relativeTransform, struct Vector3* relativeVelocity, int useCheckpoint);
void levelQueueReload();
int levelGetQueued();
void levelClearQueued();

void levelLoad(int index);
void levelGetStartLocationAndVelocity(struct Location* location, struct Vector3* velocity);
int levelLoadedFromTransition();

int levelCount();
int getChamberIndexFromLevelIndex(int levelIndex, int roomIndex);
int getLevelIndexFromChamberIndex(int chamberIndex);

int levelMaterialCount();
int levelMaterialTransparentStart();

int levelQuadIndex(struct CollisionObject* pointer);
struct Location* levelGetLocation(short index);

// The material list itself is the platform's, and the build puts one half of
// it on the include path.
#include "level_materials.h"

#endif
