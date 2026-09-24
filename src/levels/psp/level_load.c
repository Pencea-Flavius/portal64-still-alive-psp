#include "levels/levels.h"

#include "physics/collision_scene.h"
#include "levels/cutscene_runner.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/test_chambers/level_list.h"

// The PSP half of loading a level; see src/levels/n64/level_load.c.
//
// Levels are linked and resident, so nothing is copied or relocated. But the
// game writes into the definition while playing: portal cuts are reverted in
// main_psp.c, and signal-switched materials (indicator lights, door signs) are
// reset here to their exported "off" state.
static void levelResetSignalMaterials(struct LevelDefinition* level) {
    for (int i = 0; i < level->staticContentCount; ++i) {
        struct StaticContentElement* element = &level->staticContent[i];

        if (element->materialIndex == INDICATOR_LIGHTS_ON_INDEX) {
            element->materialIndex = INDICATOR_LIGHTS_INDEX;
        } else if (element->materialIndex == SIGNAGE_DOORSTATE_ON_INDEX) {
            element->materialIndex = SIGNAGE_DOORSTATE_INDEX;
        }
    }
}

void levelLoad(int index) {
    if (index < 0 || index >= LEVEL_COUNT) {
        return;
    }

    struct LevelMetadata* metadata = &gLevels[index];

    // No copy to keep or free.
    gLevelSegment = NULL;

    gCurrentLevel = metadata->levelDefinition;
    gCurrentLevelIndex = index;
    levelResetSignalMaterials(gCurrentLevel);

    collisionSceneInit(&gCollisionScene, gCurrentLevel->collisionQuads, gCurrentLevel->collisionQuadCount, &gCurrentLevel->world);
    cutsceneRunnerReset();
}

// The generated list defines gLevels, so it is included only here.
int levelCount() {
    return LEVEL_COUNT;
}
