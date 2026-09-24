#include "confirmation_dialog.h"

#include "audio/soundplayer.h"
#include "font/dejavu_sans.h"
#include "system/controller.h"
#include "system/display.h"

#include "codegen/assets/audio/clips.h"

#define DIALOG_LEFT       40
#define DIALOG_WIDTH      (SCREEN_WD - (DIALOG_LEFT * 2))
#define DIALOG_PADDING    8

#define TEXT_MARGIN       4

#define CONTENT_LEFT      (DIALOG_LEFT + DIALOG_PADDING)
#define CONTENT_WIDTH     (DIALOG_WIDTH - (DIALOG_PADDING * 2))
#define CONTENT_CENTER_X  (CONTENT_LEFT + (CONTENT_WIDTH / 2))
#define CONTENT_MARGIN    20

#define BUTTON_HEIGHT     16
#define BUTTON_MARGIN     8

void confirmationDialogInit(struct ConfirmationDialog* confirmationDialog) {
    confirmationDialog->menuOutline = menuBuildBorder(0, 0, 0, 0);
    confirmationDialog->titleText = NULL;
    confirmationDialog->messageText = NULL;
    confirmationDialog->selectedButton = NULL;
    confirmationDialog->closeCallback = NULL;
    confirmationDialog->callbackData = NULL;
    confirmationDialog->opacity = 255;
    confirmationDialog->isShown = 0;

    confirmationDialog->confirmButton = menuBuildButton(
        &gDejaVuSansFont, "",
        0,
        0,
        BUTTON_HEIGHT,
        0
    );
    confirmationDialog->cancelButton = menuBuildButton(
        &gDejaVuSansFont, "",
        0,
        0,
        BUTTON_HEIGHT,
        0
    );
}

static void confirmationDialogLayout(struct ConfirmationDialog* confirmationDialog) {
    int dialogHeight = CONTENT_MARGIN + confirmationDialog->messageText->height + BUTTON_MARGIN + BUTTON_HEIGHT + DIALOG_PADDING;
    int dialogTop = (SCREEN_HT - dialogHeight) / 2;

    menuBorderRelocate(
        confirmationDialog->menuOutline,
        DIALOG_LEFT,
        dialogTop,
        DIALOG_WIDTH,
        dialogHeight
    );
    prerenderedTextRelocate(
        confirmationDialog->titleText,
        CONTENT_LEFT,
        dialogTop + TEXT_MARGIN
    );
    prerenderedTextRelocate(
        confirmationDialog->messageText,
        CONTENT_LEFT,
        dialogTop + CONTENT_MARGIN
    );

    int buttonsWidth = confirmationDialog->confirmButton.w + confirmationDialog->cancelButton.w + BUTTON_MARGIN;
    int buttonsX = CONTENT_CENTER_X - (buttonsWidth / 2);
    int buttonsY = confirmationDialog->messageText->y + confirmationDialog->messageText->height + BUTTON_MARGIN;

    menuRelocateButton(
        &confirmationDialog->confirmButton,
        buttonsX,
        buttonsY,
        0
    );
    menuRelocateButton(
        &confirmationDialog->cancelButton,
        buttonsX + confirmationDialog->confirmButton.w + BUTTON_MARGIN,
        buttonsY,
        0
    );
}

void confirmationDialogShow(struct ConfirmationDialog* confirmationDialog, struct ConfirmationDialogParams* params) {
    if (confirmationDialog->titleText) {
        prerenderedTextFree(confirmationDialog->titleText);
    }
    if (confirmationDialog->messageText) {
        prerenderedTextFree(confirmationDialog->messageText);
    }

    confirmationDialog->titleText = menuBuildPrerenderedText(
        &gDejaVuSansFont, params->title,
        0,
        0,
        SCREEN_WD
    );
    confirmationDialog->messageText = menuBuildPrerenderedText(
        &gDejaVuSansFont, params->message,
        0,
        0,
        CONTENT_WIDTH
    );

    menuRebuildButtonText(
        &confirmationDialog->confirmButton,
        &gDejaVuSansFont,
        params->confirmLabel,
        0
    );
    menuRebuildButtonText(
        &confirmationDialog->cancelButton,
        &gDejaVuSansFont,
        params->cancelLabel,
        0
    );

    confirmationDialogLayout(confirmationDialog);

    confirmationDialog->selectedButton = &confirmationDialog->cancelButton;
    confirmationDialog->closeCallback = params->closeCallback;
    confirmationDialog->callbackData = params->callbackData;
    confirmationDialog->opacity = params->isTranslucent ? 128 : 255;
    confirmationDialog->isShown = 1;
}

static void confirmationDialogClose(struct ConfirmationDialog* confirmationDialog, int isConfirmed) {
    ConfirmationDialogCallback callback = confirmationDialog->closeCallback;
    if (callback) {
        callback(confirmationDialog->callbackData, isConfirmed);
    }

    confirmationDialog->isShown = 0;
}

enum InputCapture confirmationDialogUpdate(struct ConfirmationDialog* confirmationDialog) {
    if (controllerGetButtonsDown(0, ControllerButtonB)) {
        confirmationDialogClose(confirmationDialog, 0);
    } else if (controllerGetButtonsDown(0, ControllerButtonA)) {
        if (confirmationDialog->selectedButton == &confirmationDialog->confirmButton) {
            confirmationDialogClose(confirmationDialog, 1);
        } else {
            confirmationDialogClose(confirmationDialog, 0);
        }

        soundPlayerPlay(SOUNDS_BUTTONCLICKRELEASE, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & (ControllerDirectionLeft | ControllerDirectionRight)) {
        if (confirmationDialog->selectedButton == &confirmationDialog->confirmButton) {
            confirmationDialog->selectedButton = &confirmationDialog->cancelButton;
        } else {
            confirmationDialog->selectedButton = &confirmationDialog->confirmButton;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    return InputCaptureGrab;
}
