#include "menu/confirmation_dialog.h"

#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "system/display.h"

static struct Coloru8 gDialogColor = {164, 164, 164, 255};

static void renderButtonText(struct ConfirmationDialog* confirmationDialog, struct MenuButton* button, struct PrerenderedTextBatch* batch) {
    struct Coloru8* color = confirmationDialog->selectedButton == button ? &gColorBlack : &gColorWhite;
    prerenderedBatchAdd(batch, button->text, color);
}

void confirmationDialogRender(struct ConfirmationDialog* confirmationDialog, struct RenderState* renderState) {
    struct Coloru8 dim = {0, 0, 0, 128};

    pspRenderSetScissor(0, 0, SCREEN_WD, SCREEN_HT);

    pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&dim));

    struct Coloru8 borderColor = gDialogColor;
    borderColor.a = confirmationDialog->opacity;

    menuBorderDraw(renderState, confirmationDialog->menuOutline, pspRenderColor(&borderColor));

    if (confirmationDialog->selectedButton != NULL) {
        pspRenderFillRect(
            renderState,
            confirmationDialog->selectedButton->x,
            confirmationDialog->selectedButton->y,
            confirmationDialog->selectedButton->w,
            confirmationDialog->selectedButton->h,
            pspRenderColor(&gSelectionOrange)
        );
    }

    menuButtonDraw(renderState, &confirmationDialog->confirmButton);
    menuButtonDraw(renderState, &confirmationDialog->cancelButton);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    if (confirmationDialog->titleText) {
        prerenderedBatchAdd(batch, confirmationDialog->titleText, NULL);
    }

    if (confirmationDialog->messageText) {
        prerenderedBatchAdd(batch, confirmationDialog->messageText, NULL);
    }

    renderButtonText(confirmationDialog, &confirmationDialog->confirmButton, batch);
    renderButtonText(confirmationDialog, &confirmationDialog->cancelButton, batch);

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);
}
