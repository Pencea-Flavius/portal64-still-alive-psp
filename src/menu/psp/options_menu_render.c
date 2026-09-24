#include "menu/options_menu.h"

#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "system/display.h"

// The colours the N64 gets from the materials themselves: the overlay's env
// colour and the rounded corner atlas's. Here nothing sets state for a later
// draw, so each rectangle carries its own.
static struct Coloru8 gOverlayColor = {0, 0, 0, 85};
static struct Coloru8 gOptionsBorderColor = {164, 164, 164, 128};

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task) {
    pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&gOverlayColor));

    menuBorderDraw(renderState, options->menuOutline, pspRenderColor(&gOptionsBorderColor));

    // The tab row scrolls, so it is clipped to the frame's inside. The N64
    // spells the same window as a right edge rather than a width.
    pspRenderSetScissor(
        OPTIONS_MENU_LEFT + OPTIONS_PADDING,
        0,
        OPTIONS_MENU_WIDTH - OPTIONS_PADDING * 2,
        SCREEN_HT
    );

    tabsOutlineDraw(&options->tabs, renderState);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    tabsRenderText(&options->tabs, batch);
    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);

    pspRenderSetScissor(0, 0, SCREEN_WD, SCREEN_HT);

    switch (options->tabs.selectedTab) {
        case OptionsMenuTabsControlMapping:
            controlsMenuRender(&options->controlsMenu, renderState, task);
            break;
        case OptionsMenuTabsControlJoystick:
            joystickOptionsRender(&options->joystickOptions, renderState, task);
            break;
        case OptionsMenuTabsAudio:
            audioOptionsRender(&options->audioOptions, renderState, task);
            break;
        case OptionsMenuTabsVideo:
            videoOptionsRender(&options->videoOptions, renderState, task);
            break;
        case OptionsMenuTabsGameplay:
            gameplayOptionsRender(&options->gameplayOptions, renderState, task);
            break;
    }
}
