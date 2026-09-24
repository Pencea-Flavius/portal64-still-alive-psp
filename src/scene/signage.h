#ifndef __SCENE_SIGNAGE_H__
#define __SCENE_SIGNAGE_H__

#include "audio/soundplayer.h"
#include "levels/level_definition.h"
#include "math/transform.h"

struct Signage {
    struct Transform transform;
    short roomIndex;
    short testChamberNumber;
    short currentFrame;
    SoundId currentSoundId;
    float currentHumVolume;
    float humFadeElapTime;
};

void signageInit(struct Signage* signage, struct SignageDefinition* definition);
void signageUpdate(struct Signage* signage);
void signageActivate(struct Signage* signage);
void signageDeactivate(struct Signage* signage);

// Drawing is split per machine; the build puts one of the two on the path.
// What both halves work out first is shared.
struct Coloru8;
void signageRenderPrepare(struct Signage* signage, struct Coloru8* backlightOut, struct Coloru8* lcdOut);

#include "signage_render.h"

#endif