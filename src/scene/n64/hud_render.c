#include "scene/hud.h"

#include "font/dejavu_sans.h"
#include "font/font.h"
#include "levels/levels.h"
#include "menu/controls.h"
#include "player/player.h"
#include "savefile/savefile.h"
#include "strings/translations.h"
#include "system/display.h"
#include "util/memory.h"

#include "codegen/assets/materials/hud.h"
#include "codegen/assets/materials/ui.h"

static struct Coloru8 sCrosshairOrange = { 255, 128, 0, 255 };
static struct Coloru8 sCrosshairBlue = { 0, 128, 255, 255 };
static struct Coloru8 sSubtitleTextColor = { 255, 140, 155, 255 };

// The N64 draws these with texture rectangles, which take quarter pixels, and
// the crosshair is centred by halving the screen coordinate.
#define HUD_UPPER_X                 ((SCREEN_WD - HUD_OUTER_WIDTH  - (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_UPPER_Y                 ((SCREEN_HT - HUD_OUTER_HEIGHT - (HUD_OUTER_OFFSET_Y << 1)) << 1)
#define HUD_LOWER_X                 ((SCREEN_WD - HUD_OUTER_WIDTH  + (HUD_OUTER_OFFSET_X << 1)) << 1)
#define HUD_LOWER_Y                 ((SCREEN_HT - HUD_OUTER_HEIGHT + (HUD_OUTER_OFFSET_Y << 1)) << 1)

#define RETICLE_XMIN                ((SCREEN_WD - (8 << 1)) << 1)
#define RETICLE_YMIN                ((SCREEN_HT - (8 << 1)) << 1)

static void hudRenderOverlay(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    if (player->health < PLAYER_MAX_HEALTH) {
        float alpha = 1.0f - (player->health * (1.0f / PLAYER_MAX_HEALTH));

        gSPDisplayList(renderState->dl++, hud_overlay);
        gDPSetPrimColor(renderState->dl++, 255, 255, 255, 0, 0, (u8)(128.0f * alpha));
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, hud_overlay_revert);
    }

    if (hud->overlayTimer > 0.0f) {
        float alpha = 0.0f;

        if (hud->overlayTimer > hud->overlayFadeStartTime) {
            alpha = 1.0f;
        } else {
            alpha = hud->overlayTimer * (1.0f / hud->overlayFadeStartTime);
        }

        if (alpha >= 0.0f) {
            gSPDisplayList(renderState->dl++, hud_overlay);
            gDPSetPrimColor(renderState->dl++, 255, 255, hud->overlayColor.r, hud->overlayColor.g, hud->overlayColor.b, (u8)(255.0f * alpha));
            gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
            gSPDisplayList(renderState->dl++, hud_overlay_revert);
        }
    }
}

static void hudRenderCrosshairs(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, hud_material_list[PORTAL_CROSSHAIRS_INDEX]);

    if (player->flags & PlayerHasFirstPortalGun) {
        int leftTilePos = 0;
        int rightTilePos = HUD_OUTER_WIDTH;
        int indicatorScreenPos = -1;

        struct Coloru8* leftColor;
        struct Coloru8* rightColor;
        struct Coloru8* indicatorColor;

        if (playerIsGrabbing(player)) {
            leftColor = &gColorWhite;
            rightColor = &gColorWhite;
        } else if (!(player->flags & PlayerHasSecondPortalGun)) {
            // Unupgraded portal gun
            leftColor = &sCrosshairBlue;
            rightColor = &sCrosshairBlue;

            if (hud->flags & HudFlagsLookedPortalable1) {
                leftTilePos = HUD_OUTER_WIDTH * 2;
                rightTilePos = HUD_OUTER_WIDTH * 3;
            }
        } else {
            // Fully upgraded portal gun
            leftColor = &sCrosshairOrange;
            rightColor = &sCrosshairBlue;

            if (hud->flags & HudFlagsLookedPortalable0) {
                leftTilePos = HUD_OUTER_WIDTH * 2;
            }
            if (hud->flags & HudFlagsLookedPortalable1) {
                rightTilePos = HUD_OUTER_WIDTH * 3;
            }

            if (hud->lastPortalIndexShot == 0) {
                indicatorScreenPos = HUD_LOWER_X - 68;
                indicatorColor = leftColor;
            } else if (hud->lastPortalIndexShot == 1) {
                indicatorScreenPos = HUD_UPPER_X + 100;
                indicatorColor = rightColor;
            }
        }

        gDPSetPrimColor(renderState->dl++, 255, 255, leftColor->r, leftColor->g, leftColor->b, leftColor->a);
        gSPTextureRectangle(renderState->dl++,
            HUD_UPPER_X, HUD_UPPER_Y,
            HUD_UPPER_X + (HUD_OUTER_WIDTH << 2), HUD_UPPER_Y + (HUD_OUTER_HEIGHT << 2),
            G_TX_RENDERTILE, leftTilePos << 5, 0 << 5, 1 << 10, 1 << 10);
        gDPSetPrimColor(renderState->dl++, 255, 255, rightColor->r, rightColor->g, rightColor->b, rightColor->a);
        gSPTextureRectangle(renderState->dl++,
            HUD_LOWER_X, HUD_LOWER_Y,
            HUD_LOWER_X + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2),
            G_TX_RENDERTILE, rightTilePos << 5, 0 << 5, 1 << 10, 1 << 10);

        if (indicatorScreenPos != -1) {
            gDPSetPrimColor(renderState->dl++, 255, 255, indicatorColor->r, indicatorColor->g, indicatorColor->b, indicatorColor->a);
            gSPTextureRectangle(renderState->dl++,
                indicatorScreenPos, HUD_LOWER_Y,
                indicatorScreenPos + (HUD_OUTER_WIDTH << 2), HUD_LOWER_Y + (HUD_OUTER_HEIGHT << 2),
                G_TX_RENDERTILE, (HUD_OUTER_WIDTH * 4) << 5, 0 << 5, 1 << 10, 1 << 10);
        }
    }

    if ((!playerIsDead(player) && !(player->flags & PlayerInCutscene)) &&
        (gCurrentLevelIndex > 0 || hud->overlayTimer <= 0.0f)) {
        // Center reticle is drawn over top everything
        gSPDisplayList(renderState->dl++, hud_material_list[CENTER_RETICLE_INDEX]);
        gDPSetPrimColor(renderState->dl++, 255, 255, 210, 210, 210, 255);
        gSPTextureRectangle(renderState->dl++,
            RETICLE_XMIN, RETICLE_YMIN,
            RETICLE_XMIN + (RETICLE_WIDTH << 2), RETICLE_YMIN + (RETICLE_HEIGHT << 2),
            G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
    }
}

static void hudRenderSubtitle(char* message, float textOpacity, float backgroundOpacity, struct RenderState* renderState, enum SubtitleType subtitleType) {
    if (message == NULL || *message == '\0') {
        return;
    }

    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (SUBTITLE_MARGIN_X + SUBTITLE_PADDING) * 2);

    int textPositionX = (SUBTITLE_MARGIN_X + SUBTITLE_PADDING);
    int textPositionY = (SCREEN_HT - SUBTITLE_MARGIN_Y - SUBTITLE_PADDING) - fontRender->height;

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, (u8)(255.0f * backgroundOpacity));
    gDPFillRectangle(
        renderState->dl++,
        textPositionX - SUBTITLE_PADDING,
        textPositionY - SUBTITLE_PADDING,
        SCREEN_WD - SUBTITLE_MARGIN_X,
        SCREEN_HT - SUBTITLE_MARGIN_Y
    );
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    struct Coloru8 textColor;

    if (subtitleType == SubtitleTypeSubtitle) {
        textColor.r = sSubtitleTextColor.r;
        textColor.g = sSubtitleTextColor.g;
        textColor.b = sSubtitleTextColor.b;
    } else if (subtitleType == SubtitleTypeCaption) {
        textColor = gColorWhite;
    }

    textColor.a = (u8)(255.0f * textOpacity);

    renderState->dl = fontRendererBuildGfx(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    stackMallocFree(fontRender);
}

void hudRender(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    hudRenderOverlay(hud, player, renderState);
    hudRenderCrosshairs(hud, player, renderState);

    if (hud->subtitleOpacity > 0.0f && (gSaveData.video.flags & (VideoSaveFlagsSubtitlesEnabled | VideoSaveFlagsCaptionsEnabled)) && hud->subtitleId != StringIdNone) {
        hudRenderSubtitle(translationsGet(hud->subtitleId), hud->subtitleOpacity, hud->backgroundOpacity, renderState, hud->subtitleType);
    }

    if (hud->promptOpacity > 0.0f && hud->promptType != CutscenePromptTypeNone) {
        controlsRenderPrompt(gHudPromptActions[hud->promptType], translationsGet(gHudPromptText[hud->promptType]), hud->promptOpacity, renderState);
    }
}
