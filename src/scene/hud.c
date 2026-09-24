#include "hud.h"

#include "font/font.h"
#include "levels/levels.h"
#include "savefile/savefile.h"
#include "scene.h"
#include "strings/translations.h"
#include "system/display.h"
#include "util/frame_time.h"
#include "util/memory.h"


#define PROMPT_FADE_TIME            2.0f
#define SUBTITLE_SLOW_FADE_TIME     0.75f
#define SUBTITLE_FAST_FADE_TIME     0.25f
#define CAPTION_EXPIRE_TIME         1.5f

#define HUD_UPPER_X                 ((SCREEN_WD - HUD_OUTER_WIDTH  - (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_UPPER_Y                 ((SCREEN_HT - HUD_OUTER_HEIGHT - (HUD_OUTER_OFFSET_Y << 1)) << 1)
#define HUD_LOWER_X                 ((SCREEN_WD - HUD_OUTER_WIDTH  + (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_LOWER_Y                 ((SCREEN_HT - HUD_OUTER_HEIGHT + (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define RETICLE_XMIN                ((SCREEN_WD - (8 << 1)) << 1)
#define RETICLE_YMIN                ((SCREEN_HT - (8 << 1)) << 1)

// Which control and which string a cutscene prompt names. Both halves of the
// drawing read them.
enum ControllerAction gHudPromptActions[] = {
    ControllerActionNone,
    ControllerActionOpenPortal1,
    ControllerActionOpenPortal0,
    ControllerActionUseItem,
    ControllerActionUseItem,
    ControllerActionUseItem,
    ControllerActionDuck,
    ControllerActionMove,
    ControllerActionJump,
};

enum StringId gHudPromptText[] = {
    StringIdNone,
    HINT_GET_PORTAL_2,
    HINT_GET_PORTAL_1,
    HINT_USE_ITEMS,
    HINT_DROP_ITEMS,
    HINT_USE_SWITCHES,
    HINT_DUCK,
    HINT_MOVE,
    HINT_JUMP,
};

void hudInit(struct Hud* hud) {
    hud->promptType = CutscenePromptTypeNone;
    hud->subtitleId = StringIdNone;
    hud->queuedSubtitleId = StringIdNone;
    hud->subtitleType = SubtitleTypeNone;
    hud->queuedSubtitleType = SubtitleTypeNone;
    hud->promptOpacity = 0.0f;
    hud->subtitleOpacity = 0.0f;
    hud->backgroundOpacity = 0.0f;
    hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
    hud->flags = 0;
    hud->resolvedPrompts = 0;
    hud->lastPortalIndexShot = -1;
    hud->overlayTimer = 0.0f;
}

void hudUpdate(struct Hud* hud) {
    if (hud->overlayTimer > 0.0f) {
        hud->overlayTimer -= FIXED_DELTA_TIME;

        if (hud->overlayTimer < 0.0f) {
            hud->overlayTimer = 0.0f;
        }
    }

    if (hud->subtitleExpireTimer > 0.0f) {
        hud->subtitleExpireTimer -= FIXED_DELTA_TIME;

        if (hud->subtitleExpireTimer < 0.0f) {
            hud->subtitleExpireTimer = 0.0f;
        }
    }

    float targetPromptOpacity = (hud->flags & HudFlagsShowingPrompt) ? 1.0 : 0.0f;
    float targetSubtitleOpacity = ((hud->flags & HudFlagsShowingSubtitle) && (!(hud->flags & HudFlagsSubtitleQueued) || (hud->subtitleExpireTimer > 0.0f))) ? 0.85: 0.0f;
    float targetBackgroundOpacity = (hud->flags & HudFlagsShowingSubtitle && (!(hud->flags & HudFlagsSubtitleQueued) || (hud->subtitleExpireTimer > 0.0f))) ? 0.45: 0.0f;

    if (targetPromptOpacity != hud->promptOpacity) {
        hud->promptOpacity = mathfMoveTowards(hud->promptOpacity, targetPromptOpacity, FIXED_DELTA_TIME / PROMPT_FADE_TIME);
    }

    if (targetSubtitleOpacity != hud->subtitleOpacity) {
        hud->subtitleOpacity = mathfMoveTowards(hud->subtitleOpacity, targetSubtitleOpacity, FIXED_DELTA_TIME / hud->subtitleFadeTime);
    }

    if ((hud->subtitleOpacity <= 0.0f) && (hud->flags & HudFlagsSubtitleQueued)){
        if (!((hud->subtitleType == SubtitleTypeCaption) && (hud->queuedSubtitleType == SubtitleTypeCaption) && (hud->subtitleExpireTimer > 0.0))){
            hud->flags &= ~HudFlagsSubtitleQueued;
            hud->flags |= HudFlagsShowingSubtitle;
            hud->subtitleId = hud->queuedSubtitleId;
            hud->subtitleType = hud->queuedSubtitleType;
            hud->queuedSubtitleId = StringIdNone;
            hud->queuedSubtitleType = SubtitleTypeNone;
            if (hud->subtitleType == SubtitleTypeCaption){
                hud->subtitleExpireTimer = CAPTION_EXPIRE_TIME;
            }
        }
    } else if (hud->subtitleOpacity <= 0.0f && !(hud->flags & HudFlagsSubtitleQueued)) {
        // Allow queuing same subtitle again
        hud->subtitleId = StringIdNone;
    }

    if (hud->subtitleExpireTimer <= 0.0f && hud->subtitleType == SubtitleTypeCaption){
        hudResolveSubtitle(&gScene.hud);
    }

    if (targetBackgroundOpacity != hud->backgroundOpacity) {
        hud->backgroundOpacity = mathfMoveTowards(hud->backgroundOpacity, targetBackgroundOpacity, FIXED_DELTA_TIME / hud->subtitleFadeTime);
    }

    if (targetPromptOpacity && (hud->resolvedPrompts & (1 << hud->promptType)) != 0) {
        hudShowActionPrompt(hud, CutscenePromptTypeNone);
    }

}

void hudUpdatePortalIndicators(struct Hud* hud, struct Ray* raycastRay,  struct Vector3* playerUp) { 
    hud->flags &= ~(HudFlagsLookedPortalable0 | HudFlagsLookedPortalable1);

    if (gScene.player.flags & PlayerHasFirstPortalGun){
        if (sceneFirePortal(&gScene, raycastRay, playerUp, 0, gScene.player.body.currentRoom, 1, 1)) {
            hud->flags |= HudFlagsLookedPortalable0;
        }
        if (sceneFirePortal(&gScene, raycastRay, playerUp, 1, gScene.player.body.currentRoom, 1, 1)) {
            hud->flags |= HudFlagsLookedPortalable1;
        }
    }
}

void hudPortalFired(struct Hud* hud, int index) {
    hud->lastPortalIndexShot = index;

    if (index == 0) {
        hudResolvePrompt(hud, CutscenePromptTypePortal0);
    }
 
    if (index == 1) {
        hudResolvePrompt(hud, CutscenePromptTypePortal1);
    }
}

void hudShowActionPrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    if (promptType == CutscenePromptTypeNone || (hud->resolvedPrompts & (1 << promptType)) != 0) {
        hud->flags &= ~HudFlagsShowingPrompt;
        return;
    }

    hud->flags |= HudFlagsShowingPrompt;
    hud->promptType = promptType;
}

void hudShowSubtitle(struct Hud* hud, enum StringId subtitleId, enum SubtitleType subtitleType) {
    if (!(gSaveData.video.flags & (VideoSaveFlagsSubtitlesEnabled | VideoSaveFlagsCaptionsEnabled))) {
        return;
    }
    if (subtitleId == hud->subtitleId) {
        return;
    }

    if (subtitleType == SubtitleTypeNone) {
        hud->flags &= ~HudFlagsShowingSubtitle;
        hud->flags &= ~HudFlagsSubtitleQueued;
        hud->queuedSubtitleType = SubtitleTypeNone;
        hud->queuedSubtitleId = StringIdNone;
        hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
        return;
    }
    else if (subtitleType == SubtitleTypeCaption) {
        if (!(gSaveData.video.flags & VideoSaveFlagsCaptionsEnabled)){
            return;
        }
        if ((hud->flags & HudFlagsShowingSubtitle) && ((hud->subtitleType > subtitleType) || (hud->queuedSubtitleType > subtitleType))){
            return; // dont push off screen a higher importance subtitle
        }
        else if ((hud->flags & HudFlagsShowingSubtitle) && (hud->subtitleType <= subtitleType)){
            hud->flags |= HudFlagsSubtitleQueued;
            hud->queuedSubtitleId = subtitleId;
            hud->queuedSubtitleType = subtitleType;
            hud->subtitleFadeTime = SUBTITLE_FAST_FADE_TIME;
        }
        else{
            hud->flags |= HudFlagsShowingSubtitle;
            hud->subtitleId = subtitleId;
            hud->subtitleType = subtitleType;
            hud->queuedSubtitleType = SubtitleTypeNone;
            hud->queuedSubtitleId = StringIdNone;
            hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
            hud->subtitleExpireTimer = CAPTION_EXPIRE_TIME;
        }
        return;
    }
    else if (subtitleType == SubtitleTypeSubtitle) {
        if (hud->flags & HudFlagsShowingSubtitle){
            hud->flags |= HudFlagsSubtitleQueued;
            hud->queuedSubtitleId = subtitleId;
            hud->queuedSubtitleType = subtitleType;
            hud->subtitleFadeTime = SUBTITLE_FAST_FADE_TIME;
        }
        else{
            hud->flags |= HudFlagsShowingSubtitle;
            hud->flags &= ~HudFlagsSubtitleQueued;
            hud->subtitleId = subtitleId;
            hud->subtitleType = subtitleType;
            hud->queuedSubtitleType = SubtitleTypeNone;
            hud->queuedSubtitleId = StringIdNone;
            hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
        }
        return;
    }
}

void hudResolvePrompt(struct Hud* hud, enum CutscenePromptType promptType) {
    hud->resolvedPrompts |= (1 << promptType);
}

void hudResolveSubtitle(struct Hud* hud) {

    hud->flags &= ~HudFlagsShowingSubtitle;
    hud->subtitleFadeTime = SUBTITLE_SLOW_FADE_TIME;
}

void hudShowColoredOverlay(struct Hud* hud, struct Coloru8* color, float duration, float fadeStartTime) {
    hud->overlayColor = *color;
    hud->overlayTimer = duration;
    hud->overlayFadeStartTime = fadeStartTime;
}

int hudOverlayVisible(struct Hud* hud, struct Player* player) {
    return hud->overlayTimer > 0.0f || player->health < PLAYER_MAX_HEALTH;
}
