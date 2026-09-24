#ifndef __SCENE_HUD_H__
#define __SCENE_HUD_H__

#include "controls/controller_actions.h"
#include "graphics/color.h"
#include "graphics/renderstate.h"
#include "player/player.h"

#include "codegen/assets/strings/strings.h"

enum SubtitleType {
    SubtitleTypeNone,
    SubtitleTypeCaption,
    SubtitleTypeSubtitle,
};

enum HudFlags {
    HudFlagsLookedPortalable0 = (1 << 0),
    HudFlagsLookedPortalable1 = (1 << 1),
    HudFlagsShowingPrompt = (1 << 2),
    HudFlagsShowingSubtitle = (1 << 3),
    HudFlagsSubtitleQueued = (1 << 4),
};

// The crosshair and subtitle box, in pixels; each half converts them.
#define HUD_OUTER_WIDTH             24
#define HUD_OUTER_HEIGHT            32

#define HUD_OUTER_OFFSET_X          3
#define HUD_OUTER_OFFSET_Y          5

#define RETICLE_WIDTH               16
#define RETICLE_HEIGHT              16

#define SUBTITLE_MARGIN_X           17
#define SUBTITLE_MARGIN_Y           11
#define SUBTITLE_PADDING            6

struct Hud {
    enum CutscenePromptType promptType;
    enum StringId subtitleId;
    enum StringId queuedSubtitleId;
    enum SubtitleType subtitleType;
    enum SubtitleType queuedSubtitleType;
    float promptOpacity;
    float subtitleOpacity;
    float backgroundOpacity;

    float subtitleFadeTime;
    float subtitleExpireTimer;

    struct Coloru8 overlayColor;
    float overlayTimer;
    float overlayFadeStartTime;

    u16 flags;
    u16 resolvedPrompts;

    u8 lastPortalIndexShot;
};

void hudInit(struct Hud* hud);

void hudUpdate(struct Hud* hud);
void hudUpdatePortalIndicators(struct Hud* hud, struct Ray* raycastRay,  struct Vector3* playerUp);

void hudPortalFired(struct Hud* hud, int index);
void hudShowActionPrompt(struct Hud* hud, enum CutscenePromptType promptType);
void hudResolvePrompt(struct Hud* hud, enum CutscenePromptType promptType);
void hudShowSubtitle(struct Hud* hud, enum StringId subtitleId, enum SubtitleType subtitleType);
void hudResolveSubtitle(struct Hud* hud);
void hudShowColoredOverlay(struct Hud* hud, struct Coloru8* color, float duration, float fadeStartTime);

int hudOverlayVisible(struct Hud* hud, struct Player* player);

// A cutscene prompt's control and string, for both halves.
extern enum ControllerAction gHudPromptActions[];
extern enum StringId gHudPromptText[];

// Drawing is declared in the platform's half.
#include "hud_render.h"

#endif