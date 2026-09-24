#include "menu/landing_menu.h"

#include "font/dejavu_sans.h"
#include "system/display.h"

#include "codegen/assets/materials/ui.h"

// The logo is three texture rectangles out of one image: the P, the O that is
// tinted, and the rest of the word.
Gfx portal_logo_gfx[] = {
    gsSPTextureRectangle(
        PORTAL_LOGO_X << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        0, 0,
        0x400, 0x400
    ),
    gsDPPipeSync(),
    gsDPSetEnvColor(60, 189, 236, 255),
    gsSPTextureRectangle(
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH) << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        PORTAL_LOGO_P_WIDTH << 5, 0,
        0x400, 0x400
    ),
    gsDPPipeSync(),
    gsDPSetEnvColor(255, 255, 255, 255),
    gsSPTextureRectangle(
        (PORTAL_LOGO_X + PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 2,
        PORTAL_LOGO_Y << 2,
        (PORTAL_LOGO_X + PORTAL_LOGO_WIDTH) << 2,
        (PORTAL_LOGO_Y + PORTAL_LOGO_HEIGHT) << 2,
        G_TX_RENDERTILE,
        (PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH) << 5, 0,
        0x400, 0x400
    ),
    gsSPEndDisplayList(),
};

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    if (landingMenu->darkenBackground) {
        gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
        gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
        gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    }

    gSPDisplayList(renderState->dl++, ui_material_list[PORTAL_LOGO_INDEX]);
    gSPDisplayList(renderState->dl++, portal_logo_gfx);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[PORTAL_LOGO_INDEX]);

	int paddingDepthY = 2;
	int paddingDepthX = 4;
	if (landingMenu->optionCount > PACKED_MENU_THRESHOLD){
		paddingDepthY = 0;
	}

    int maxTextWidth = 0;
    for (int i = 0; i < landingMenu->optionCount; ++i) {
        if (landingMenu->optionText[i]->width > maxTextWidth) {
            maxTextWidth = landingMenu->optionText[i]->width;
        }
    }

    gSPDisplayList(renderState->dl++, ui_material_list[ORANGE_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++,
        landingMenu->optionText[landingMenu->selectedItem]->x - paddingDepthX,
        landingMenu->optionText[landingMenu->selectedItem]->y - paddingDepthY,
        landingMenu->optionText[landingMenu->selectedItem]->x + maxTextWidth + paddingDepthX,
        landingMenu->optionText[landingMenu->selectedItem]->y + getCurrentStrideValue(landingMenu) - paddingDepthY);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ORANGE_TRANSPARENT_OVERLAY_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    for (int i = 0; i < landingMenu->optionCount; ++i) {
        prerenderedBatchAdd(batch, landingMenu->optionText[i], &gColorWhite);
    }
    prerenderedBatchAdd(batch, landingMenu->versionText, &gHalfTransparentWhite);

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);
}

