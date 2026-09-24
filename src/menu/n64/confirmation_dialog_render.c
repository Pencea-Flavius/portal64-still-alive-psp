#include "menu/confirmation_dialog.h"

#include "font/dejavu_sans.h"
#include "system/display.h"

#include "codegen/assets/materials/ui.h"

static struct Coloru8 gDialogColor = {164, 164, 164, 255};

static void renderButtonText(struct ConfirmationDialog* confirmationDialog, struct MenuButton* button, struct PrerenderedTextBatch* batch) {
    struct Coloru8* color = confirmationDialog->selectedButton == button ? &gColorBlack : &gColorWhite;
    prerenderedBatchAdd(batch, button->text, color);
}

void confirmationDialogRender(struct ConfirmationDialog* confirmationDialog, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);
    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gDialogColor.r, gDialogColor.g, gDialogColor.b, confirmationDialog->opacity);
    gSPDisplayList(renderState->dl++, confirmationDialog->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);

    if (confirmationDialog->selectedButton != NULL) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        gDPFillRectangle(
            renderState->dl++,
            confirmationDialog->selectedButton->x,
            confirmationDialog->selectedButton->y,
            confirmationDialog->selectedButton->x + confirmationDialog->selectedButton->w,
            confirmationDialog->selectedButton->y + confirmationDialog->selectedButton->h
        );
    }

    gSPDisplayList(renderState->dl++, confirmationDialog->confirmButton.outline);
    gSPDisplayList(renderState->dl++, confirmationDialog->cancelButton.outline);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    if (confirmationDialog->titleText) {
        prerenderedBatchAdd(batch, confirmationDialog->titleText, NULL);
    }
    if (confirmationDialog->messageText) {
        prerenderedBatchAdd(batch, confirmationDialog->messageText, NULL);
    }

    renderButtonText(confirmationDialog, &confirmationDialog->confirmButton, batch);
    renderButtonText(confirmationDialog, &confirmationDialog->cancelButton, batch);

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}
