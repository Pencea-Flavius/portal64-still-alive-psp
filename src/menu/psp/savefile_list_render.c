#include "menu/savefile_list.h"

#include "controls/controller_actions.h"
#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "menu/controls.h"
#include "system/display.h"

#include <pspgu.h>

static struct Coloru8 gOverlayColor = {0, 0, 0, 85};
static struct Coloru8 gListBackColor = {0, 0, 0, 85};
static struct Coloru8 gMenuBorderColor = {164, 164, 164, 128};

void savefileListSlotRenderInit(struct SavefileListSlot* slot) {
    // The thumbnail is framebuffer pixels, and the display is 5551.
    pspImageInit(&slot->render.image, SAVE_SLOT_IMAGE_W, SAVE_SLOT_IMAGE_H, GU_PSM_5551);
}

void savefileListSlotImageUpdate(struct SavefileListSlot* slot) {
    pspImageUpload(&slot->render.image, slot->imageData);
}

static void savefileListRenderControls(struct SavefileListMenu* savefileList, struct RenderState* renderState) {
    if (savefileList->numberOfSaves == 0) {
        return;
    }

    struct SavefileInfo* selectedSave = &savefileList->savefileInfo[savefileList->selectedSave];
    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

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

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);
}

static void savefileListRenderSlotImage(struct SavefileListSlot* slot, struct RenderState* renderState) {
    pspImageDraw(
        renderState, &slot->render.image,
        slot->x + BORDER_THICKNESS, slot->y + BORDER_THICKNESS,
        SAVE_SLOT_RENDER_W, SAVE_SLOT_RENDER_H,
        0xFFFFFFFF
    );
}

void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task) {
    (void)task;

    pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&gOverlayColor));

    menuBorderDraw(renderState, savefileList->menuOutline, pspRenderColor(&gMenuBorderColor));

    pspRenderFillRect(renderState, CONTENT_X, CONTENT_Y, CONTENT_WIDTH, CONTENT_HEIGHT, pspRenderColor(&gListBackColor));

    pspRenderSetScissor(CONTENT_X, CONTENT_Y, CONTENT_WIDTH, CONTENT_HEIGHT);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        menuSolidBorderDraw(
            renderState,
            slot->border,
            pspRenderColor(menuRenderColor(
                savefileList->indexOffset + i == savefileList->selectedSave, &gSelectionOrange, &gColorBlack))
        );
    }

    pspRenderSetScissor(0, 0, SCREEN_WD, SCREEN_HT);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    if (savefileList->savefileListTitleText) {
        prerenderedBatchAdd(batch, savefileList->savefileListTitleText, NULL);
    }

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);

    savefileListRenderControls(savefileList, renderState);

    pspRenderSetScissor(CONTENT_X, CONTENT_Y, CONTENT_WIDTH, CONTENT_HEIGHT);

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

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        struct SavefileListSlot* slot = &savefileList->slots[i];

        if (slot->slotIndex < 0) {
            continue;
        }

        savefileListRenderSlotImage(slot, renderState);
    }

    if (savefileList->confirmationDialog.isShown) {
        confirmationDialogRender(&savefileList->confirmationDialog, renderState);
    }

    pspRenderSetScissor(0, 0, SCREEN_WD, SCREEN_HT);
}
