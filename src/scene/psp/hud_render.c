#include "scene/hud.h"

#include "font/dejavu_sans.h"
#include "font/font.h"
#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
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

// The same places the N64 puts them, in pixels rather than quarter pixels.
// Halving the screen coordinate is how the N64 centres the crosshair, which
// reads as this once the shift is undone.
#define HUD_UPPER_X     (((SCREEN_WD - HUD_OUTER_WIDTH) / 2) - HUD_OUTER_OFFSET_X)
#define HUD_UPPER_Y     (((SCREEN_HT - HUD_OUTER_HEIGHT) / 2) - HUD_OUTER_OFFSET_Y)
#define HUD_LOWER_X     (((SCREEN_WD - HUD_OUTER_WIDTH) / 2) + HUD_OUTER_OFFSET_X)
#define HUD_LOWER_Y     (((SCREEN_HT - HUD_OUTER_HEIGHT) / 2) + HUD_OUTER_OFFSET_Y)

#define RETICLE_XMIN    ((SCREEN_WD - RETICLE_WIDTH) / 2)
#define RETICLE_YMIN    ((SCREEN_HT - RETICLE_HEIGHT) / 2)

// The N64 sets a prim colour and then draws; here every sprite carries its own.
static void hudDrawTile(
    struct RenderState* renderState,
    const struct PspMaterial* material,
    int x, int y, int width, int height,
    int tileX,
    struct Coloru8* color
) {
    if (!material->texture) {
        return;
    }

    pspRenderTextureRect(
        renderState, material,
        x, y, width, height,
        tileX, 0,
        tileX + width, height,
        pspRenderColor(color)
    );
}

static void hudRenderOverlay(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    if (player->health < PLAYER_MAX_HEALTH) {
        float alpha = 1.0f - (player->health * (1.0f / PLAYER_MAX_HEALTH));

        struct Coloru8 hurt = {255, 0, 0, (unsigned char)(128.0f * alpha)};
        pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&hurt));
    }

    if (hud->overlayTimer > 0.0f) {
        float alpha = 1.0f;

        if (hud->overlayTimer <= hud->overlayFadeStartTime) {
            alpha = hud->overlayTimer * (1.0f / hud->overlayFadeStartTime);
        }

        if (alpha >= 0.0f) {
            struct Coloru8 overlay = hud->overlayColor;
            overlay.a = (unsigned char)(255.0f * alpha);
            pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&overlay));
        }
    }
}

static void hudRenderCrosshairs(struct Hud* hud, struct Player* player, struct RenderState* renderState) {
    const struct PspMaterial* crosshairs = hud_material_list[PORTAL_CROSSHAIRS_INDEX];

    if (player->flags & PlayerHasFirstPortalGun) {
        int leftTilePos = 0;
        int rightTilePos = HUD_OUTER_WIDTH;
        int indicatorScreenPos = -1;

        struct Coloru8* leftColor;
        struct Coloru8* rightColor;
        struct Coloru8* indicatorColor = &gColorWhite;

        if (playerIsGrabbing(player)) {
            leftColor = &gColorWhite;
            rightColor = &gColorWhite;
        } else if (!(player->flags & PlayerHasSecondPortalGun)) {
            leftColor = &sCrosshairBlue;
            rightColor = &sCrosshairBlue;

            if (hud->flags & HudFlagsLookedPortalable1) {
                leftTilePos = HUD_OUTER_WIDTH * 2;
                rightTilePos = HUD_OUTER_WIDTH * 3;
            }
        } else {
            leftColor = &sCrosshairOrange;
            rightColor = &sCrosshairBlue;

            if (hud->flags & HudFlagsLookedPortalable0) {
                leftTilePos = HUD_OUTER_WIDTH * 2;
            }
            if (hud->flags & HudFlagsLookedPortalable1) {
                rightTilePos = HUD_OUTER_WIDTH * 3;
            }

            if (hud->lastPortalIndexShot == 0) {
                indicatorScreenPos = HUD_LOWER_X - 17;
                indicatorColor = leftColor;
            } else if (hud->lastPortalIndexShot == 1) {
                indicatorScreenPos = HUD_UPPER_X + 25;
                indicatorColor = rightColor;
            }
        }

        hudDrawTile(renderState, crosshairs, HUD_UPPER_X, HUD_UPPER_Y, HUD_OUTER_WIDTH, HUD_OUTER_HEIGHT, leftTilePos, leftColor);
        hudDrawTile(renderState, crosshairs, HUD_LOWER_X, HUD_LOWER_Y, HUD_OUTER_WIDTH, HUD_OUTER_HEIGHT, rightTilePos, rightColor);

        if (indicatorScreenPos != -1) {
            hudDrawTile(
                renderState, crosshairs,
                indicatorScreenPos, HUD_LOWER_Y,
                HUD_OUTER_WIDTH, HUD_OUTER_HEIGHT,
                HUD_OUTER_WIDTH * 4, indicatorColor
            );
        }
    }

    if ((!playerIsDead(player) && !(player->flags & PlayerInCutscene)) &&
        (gCurrentLevelIndex > 0 || hud->overlayTimer <= 0.0f)) {
        // Centre reticle is drawn over the top of everything.
        struct Coloru8 reticleColor = {210, 210, 210, 255};

        hudDrawTile(
            renderState, hud_material_list[CENTER_RETICLE_INDEX],
            RETICLE_XMIN, RETICLE_YMIN,
            RETICLE_WIDTH, RETICLE_HEIGHT,
            0, &reticleColor
        );
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

    struct Coloru8 background = {0, 0, 0, (unsigned char)(255.0f * backgroundOpacity)};

    pspRenderFillRect(
        renderState,
        textPositionX - SUBTITLE_PADDING,
        textPositionY - SUBTITLE_PADDING,
        (SCREEN_WD - SUBTITLE_MARGIN_X) - (textPositionX - SUBTITLE_PADDING),
        (SCREEN_HT - SUBTITLE_MARGIN_Y) - (textPositionY - SUBTITLE_PADDING),
        pspRenderColor(&background)
    );

    struct Coloru8 textColor = gColorWhite;

    if (subtitleType == SubtitleTypeSubtitle) {
        textColor.r = sSubtitleTextColor.r;
        textColor.g = sSubtitleTextColor.g;
        textColor.b = sSubtitleTextColor.b;
    }

    textColor.a = (unsigned char)(255.0f * textOpacity);

    fontRendererDraw(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState);

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
