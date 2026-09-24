#include "menu/landing_menu.h"

#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "system/display.h"

#include "codegen/assets/materials/ui.h"

static struct Coloru8 gOverlayColor = {0, 0, 0, 85};
static struct Coloru8 gLogoColor = {255, 255, 255, 255};
// The blue the N64 sets as an env colour between the logo's rectangles.
static struct Coloru8 gLogoOColor = {60, 189, 236, 255};
static struct Coloru8 gSelectionColor = {255, 156, 0, 142};

// The same three rectangles the N64 builds a display list out of: the P, the
// O that is tinted, and the rest of the word. Nothing is retained, so they are
// laid out again each frame from the sizes in the header.
static void landingMenuDrawLogo(struct RenderState* renderState) {
    const struct PspMaterial* material = ui_material_list[PORTAL_LOGO_INDEX];

    int spans[3][2] = {
        {0,                                     PORTAL_LOGO_P_WIDTH},
        {PORTAL_LOGO_P_WIDTH,                   PORTAL_LOGO_O_WIDTH},
        {PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH,
         PORTAL_LOGO_WIDTH - (PORTAL_LOGO_P_WIDTH + PORTAL_LOGO_O_WIDTH)},
    };

    for (int i = 0; i < 3; ++i) {
        int offset = spans[i][0];
        int width = spans[i][1];

        pspRenderTextureRect(
            renderState, material,
            PORTAL_LOGO_X + offset, PORTAL_LOGO_Y,
            width, PORTAL_LOGO_HEIGHT,
            offset, 0,
            offset + width, PORTAL_LOGO_HEIGHT,
            pspRenderColor(i == 1 ? &gLogoOColor : &gLogoColor)
        );
    }
}

void landingMenuRender(struct LandingMenu* landingMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    (void)task;

    if (landingMenu->darkenBackground) {
        pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&gOverlayColor));
    }

    landingMenuDrawLogo(renderState);

    int paddingDepthY = 2;
    int paddingDepthX = 4;

    if (landingMenu->optionCount > PACKED_MENU_THRESHOLD) {
        paddingDepthY = 0;
    }

    int maxTextWidth = 0;

    for (int i = 0; i < landingMenu->optionCount; ++i) {
        if (landingMenu->optionText[i]->width > maxTextWidth) {
            maxTextWidth = landingMenu->optionText[i]->width;
        }
    }

    struct PrerenderedText* selected = landingMenu->optionText[landingMenu->selectedItem];

    pspRenderFillRect(
        renderState,
        selected->x - paddingDepthX,
        selected->y - paddingDepthY,
        maxTextWidth + paddingDepthX * 2,
        getCurrentStrideValue(landingMenu),
        pspRenderColor(&gSelectionColor)
    );

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    for (int i = 0; i < landingMenu->optionCount; ++i) {
        prerenderedBatchAdd(batch, landingMenu->optionText[i], &gColorWhite);
    }

    prerenderedBatchAdd(batch, landingMenu->versionText, &gHalfTransparentWhite);

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);
}
