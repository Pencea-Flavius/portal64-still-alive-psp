#include "menu/savefile_list.h"

#include "controls/controller_actions.h"
#include "font/dejavu_sans.h"
#include "menu/controls.h"
#include "system/display.h"

#include "codegen/assets/materials/ui.h"

// The tile is loaded from the shared copy each frame, so there is nothing to
// set up and nothing to refresh when that copy changes.
void savefileListSlotRenderInit(struct SavefileListSlot* slot) {
    (void)slot;
}

void savefileListSlotImageUpdate(struct SavefileListSlot* slot) {
    (void)slot;
}

static void savefileListRenderControls(struct SavefileListMenu* savefileList, struct RenderState* renderState) {
    if (savefileList->numberOfSaves == 0) {
        return;
    }

    struct SavefileInfo* selectedSave = &savefileList->savefileInfo[savefileList->selectedSave];
    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);

    if (savefileList->confirmText) {
        controlsRenderInputIcon(
            ControllerActionInputAButton,
            savefileList->confirmText->x - CONTROL_TEXT_PADDING - 2,
            savefileList->confirmText->y,
            renderState
        );
        prerenderedBatchAdd(batch, savefileList->confirmText, NULL);
    }
    if (savefileList->deleteText && !selectedSave->isFree) {
        controlsRenderInputIcon(
            ControllerActionInputZTrig,
            savefileList->deleteText->x - CONTROL_TEXT_PADDING - 1,
            savefileList->deleteText->y,
            renderState
        );
        prerenderedBatchAdd(batch, savefileList->deleteText, NULL);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
}

void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, savefileList->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, savefileList->indexOffset + i == savefileList->selectedSave, &gSelectionOrange, &gColorBlack);

        renderStateAppendDL(renderState, slot->border);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    if (savefileList->savefileListTitleText) {
        prerenderedBatchAdd(batch, savefileList->savefileListTitleText, NULL);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    savefileListRenderControls(savefileList, renderState);

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, CONTENT_X, CONTENT_Y, CONTENT_X + CONTENT_WIDTH, CONTENT_Y + CONTENT_HEIGHT);

    batch = prerenderedBatchStart();

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        struct Coloru8* color = savefileList->indexOffset + i == savefileList->selectedSave ? &gSelectionOrange : &gColorWhite;

        prerenderedBatchAdd(batch, slot->testChamberText, color);

        if (slot->gameId) {
            prerenderedBatchAdd(batch, slot->gameId, color);
        }
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);


    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[IMAGE_COPY_INDEX]);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        gDPLoadTextureTile(
            renderState->dl++,
            K0_TO_PHYS(slot->imageData),
            G_IM_FMT_RGBA, G_IM_SIZ_16b,
            SAVE_SLOT_IMAGE_W, SAVE_SLOT_IMAGE_H,
            0, 0,
            SAVE_SLOT_IMAGE_W-1, SAVE_SLOT_IMAGE_H-1,
            0,
            G_TX_CLAMP, G_TX_CLAMP,
            G_TX_NOMASK, G_TX_NOMASK,
            G_TX_NOLOD, G_TX_NOLOD
        );
        
        gSPTextureRectangle(
            renderState->dl++,
            (slot->x + BORDER_THICKNESS) << 2, (slot->y + BORDER_THICKNESS) << 2,
            (slot->x + BORDER_THICKNESS + SAVE_SLOT_RENDER_W) << 2, 
            (slot->y + BORDER_THICKNESS + SAVE_SLOT_RENDER_H) << 2,
            G_TX_RENDERTILE, 
            0, 0, 
            (SAVE_SLOT_IMAGE_W << 10) / SAVE_SLOT_RENDER_W, (SAVE_SLOT_IMAGE_H << 10) / SAVE_SLOT_RENDER_H
        );
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[IMAGE_COPY_INDEX]);

    if (savefileList->confirmationDialog.isShown) {
        confirmationDialogRender(&savefileList->confirmationDialog, renderState);
    }

    gDPPipeSync(renderState->dl++);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}
