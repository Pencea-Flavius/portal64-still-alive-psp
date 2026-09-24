#include "menu/options_menu.h"

#include "font/dejavu_sans.h"
#include "system/display.h"

#include "codegen/assets/materials/ui.h"

void optionsMenuRender(struct OptionsMenu* options, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, options->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gDPSetScissor(renderState->dl++, 
        G_SC_NON_INTERLACE, 
        OPTIONS_MENU_LEFT + OPTIONS_PADDING, 
        0,
        OPTIONS_MENU_LEFT + OPTIONS_MENU_WIDTH - OPTIONS_PADDING, 
        SCREEN_HT
    );

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, options->tabs.tabOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    tabsRenderText(&options->tabs, batch);
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    gDPSetScissor(renderState->dl++, 
        G_SC_NON_INTERLACE, 
        0, 0,
        SCREEN_WD, SCREEN_HT
    );

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
