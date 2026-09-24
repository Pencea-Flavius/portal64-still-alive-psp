#include "savefile_list.h"

#include "audio/soundplayer.h"
#include "font/dejavu_sans.h"
#include "strings/translations.h"
#include "system/controller.h"
#include "system/display.h"
#include "text_manipulation.h"
#include "util/memory.h"
#include "util/string.h"

#include "codegen/assets/audio/clips.h"
#include "codegen/assets/strings/strings.h"

void savefileListSlotUseInfo(struct SavefileListSlot* savefileListSlot, struct SavefileInfo* savefileInfo, int x, int y) {
    if (savefileListSlot->testChamberText) {
        menuFreePrerenderedDeferred(savefileListSlot->testChamberText);
        savefileListSlot->testChamberText = NULL;
    }

    if (savefileListSlot->gameId) {
        menuFreePrerenderedDeferred(savefileListSlot->testChamberText);
        savefileListSlot->gameId = NULL;
    }

    char message[64];
    textManipTestChamberMessage(message, savefileInfo->testChamberNumber);
    savefileListSlot->testChamberText = menuBuildPrerenderedText(&gDejaVuSansFont, message, x + BORDER_WIDTH + 8, y, 120);

    if (savefileInfo->savefileName) {
        strCopy(message, savefileInfo->savefileName);
    } else {
        textManipSubjectMessage(message, savefileInfo->testSubjectNumber);
    }

    savefileListSlot->gameId = menuBuildPrerenderedText(&gDejaVuSansFont, message, x + BORDER_WIDTH + 8, y + savefileListSlot->testChamberText->height + 4, 120);

    menuSolidBorderRelocate(
        savefileListSlot->border,
        x, y,
        BORDER_WIDTH, BORDER_HEIGHT,
        x + BORDER_THICKNESS, y + BORDER_THICKNESS,
        SAVE_SLOT_IMAGE_W * 2, SAVE_SLOT_IMAGE_H * 2
    );
    savefileListSlot->slotIndex = savefileInfo->slotIndex;

    savefileCopySlotImage(savefileInfo->slotIndex, savefileListSlot->imageData);
    savefileListSlotImageUpdate(savefileListSlot);
    savefileListSlot->x = x;
    savefileListSlot->y = y;
}

void savefileListSlotInit(struct SavefileListSlot* savefileListSlot, int x, int y) {
    savefileListSlot->testChamberText = NULL;
    savefileListSlot->gameId = NULL;
    savefileListSlot->border = menuBuildSolidBorder(
        x, y, BORDER_WIDTH, BORDER_HEIGHT,
        x + BORDER_THICKNESS, y + BORDER_THICKNESS, 82, 48
    );

    savefileListSlot->x = x;
    savefileListSlot->y = y;
    savefileListSlot->imageData = malloc(SAVE_SLOT_IMAGE_SIZE);
    savefileListSlot->slotIndex = -1;

    savefileListSlotRenderInit(savefileListSlot);
}

#define SCROLLED_ROW_Y(rowIndex, scrollOffset) (LOAD_GAME_TOP + (rowIndex) * ROW_HEIGHT + FILE_OFFSET_Y + (scrollOffset))

void savefileListMenuSetScroll(struct SavefileListMenu* savefileList, int amount) {
    int minScroll = CONTENT_HEIGHT - ROW_HEIGHT * savefileList->numberOfSaves - 8;

    if (amount < minScroll) {
        amount = minScroll;
    }

    if (amount > 0) {
        amount = 0;
    }

    savefileList->scrollOffset = amount;
    savefileList->indexOffset = -amount / ROW_HEIGHT;

    int i;

    for (i = 0; i + savefileList->indexOffset < savefileList->numberOfSaves && i < MAX_VISIBLE_SLOTS; ++i) {
        savefileListSlotUseInfo(
            &savefileList->slots[i], 
            &savefileList->savefileInfo[i + savefileList->indexOffset],
            LOAD_GAME_LEFT + FILE_OFFSET_X,
            SCROLLED_ROW_Y(i + savefileList->indexOffset, savefileList->scrollOffset)
        );
    }

    for (; i < MAX_VISIBLE_SLOTS; ++i) {
        savefileList->slots[i].slotIndex = -1;
    }
}

void savefileListMenuInit(struct SavefileListMenu* savefileList) {
    savefileList->menuOutline = menuBuildBorder(LOAD_GAME_LEFT, LOAD_GAME_TOP, SCREEN_WD - LOAD_GAME_LEFT * 2, SCREEN_HT - LOAD_GAME_TOP * 2);
    savefileList->savefileListTitleText = NULL;
    savefileList->deleteText = NULL;
    savefileList->confirmText = NULL;

    savefileList->numberOfSaves = 3;
    savefileList->scrollOffset = 0;

    for (int i = 0; i < MAX_VISIBLE_SLOTS; ++i) {
        savefileListSlotInit(
            &savefileList->slots[i], 
            LOAD_GAME_LEFT + FILE_OFFSET_X, 
            LOAD_GAME_TOP + i * ROW_HEIGHT + FILE_OFFSET_Y
        );
    }

    confirmationDialogInit(&savefileList->confirmationDialog);
}

void savefileUseList(struct SavefileListMenu* savefileList, char* title, char* confirmLabel, struct SavefileInfo* savefileInfo, int slotCount) {
    if (savefileList->savefileListTitleText) {
        prerenderedTextFree(savefileList->savefileListTitleText);
    }
    if (savefileList->deleteText) {
        prerenderedTextFree(savefileList->deleteText);
    }
    if (savefileList->confirmText) {
        prerenderedTextFree(savefileList->confirmText);
    }
    
    savefileList->savefileListTitleText = menuBuildPrerenderedText(
        &gDejaVuSansFont,
        title,
        CONTENT_X,
        LOAD_GAME_TOP + 4,
        SCREEN_WD
    );
    savefileList->deleteText = menuBuildPrerenderedText(&gDejaVuSansFont,
        translationsGet(GAMEUI_DELETE),
        CONTENT_X + CONTROL_TEXT_PADDING,
        CONTENT_Y + CONTENT_HEIGHT + CONTROL_TEXT_MARGIN,
        120
    );
    savefileList->confirmText = menuBuildPrerenderedText(
        &gDejaVuSansFont,
        confirmLabel,
        0,
        CONTENT_Y + CONTENT_HEIGHT + CONTROL_TEXT_MARGIN,
        120
    );
    prerenderedTextRelocate(
        savefileList->confirmText,
        CONTENT_X + CONTENT_WIDTH - savefileList->confirmText->width - 1,
        savefileList->confirmText->y
    );

    for (int i = 0; i < slotCount; ++i) {
        savefileList->savefileInfo[i] = savefileInfo[i];
    }

    savefileList->selectedSave = 0;
    savefileList->numberOfSaves = slotCount;
    
    savefileListMenuSetScroll(savefileList, 0);
}

enum InputCapture savefileListUpdate(struct SavefileListMenu* savefileList) {
    if (savefileList->confirmationDialog.isShown) {
        return confirmationDialogUpdate(&savefileList->confirmationDialog);
    }

    if (controllerGetButtonsDown(0, ControllerButtonB)) {
        return InputCaptureExit;
    }

    if (savefileList->numberOfSaves == 0) {
        return InputCapturePass;
    }

    int controllerDir = controllerGetDirectionDown(0);
    if (controllerDir & ControllerDirectionDown) {
        savefileList->selectedSave = savefileList->selectedSave + 1;

        if (savefileList->selectedSave == savefileList->numberOfSaves) {
            savefileList->selectedSave = 0;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    if (controllerDir & ControllerDirectionUp) {
        savefileList->selectedSave = savefileList->selectedSave - 1;

        if (savefileList->selectedSave < 0) {
            savefileList->selectedSave = savefileList->numberOfSaves - 1;
        }
        soundPlayerPlay(SOUNDS_BUTTONROLLOVER, 1.0f, 1.0f, NULL, NULL, SoundTypeAll);
    }

    int selectTop = SCROLLED_ROW_Y(savefileList->selectedSave, savefileList->scrollOffset) - 8;
    int selectBottom = selectTop + ROW_HEIGHT + 16;

    if (selectBottom > CONTENT_Y + CONTENT_HEIGHT) {
        savefileListMenuSetScroll(savefileList, savefileList->scrollOffset + CONTENT_Y + CONTENT_HEIGHT - selectBottom);
    }

    if (selectTop < CONTENT_Y) {
        savefileListMenuSetScroll(savefileList, savefileList->scrollOffset + CONTENT_Y - selectTop);
    }

    return InputCapturePass;
}

int savefileGetSlot(struct SavefileListMenu* savefileList) {
    return savefileList->savefileInfo[savefileList->selectedSave].slotIndex;
}

void savefileListConfirmDeletion(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData) {
    struct ConfirmationDialogParams dialogParams = {
        translationsGet(GAMEUI_CONFIRMDELETESAVEGAME_TITLE),
        translationsGet(GAMEUI_CONFIRMDELETESAVEGAME_INFO),
        translationsGet(GAMEUI_CONFIRMDELETESAVEGAME_OK),
        translationsGet(GAMEUI_CANCEL),
        0,
        callback,
        callbackData
    };

    confirmationDialogShow(&savefileList->confirmationDialog, &dialogParams);
}

void savefileListConfirmOverwrite(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData) {
    struct ConfirmationDialogParams dialogParams = {
        translationsGet(GAMEUI_CONFIRMOVERWRITESAVEGAME_TITLE),
        translationsGet(GAMEUI_CONFIRMOVERWRITESAVEGAME_INFO),
        translationsGet(GAMEUI_CONFIRMOVERWRITESAVEGAME_OK),
        translationsGet(GAMEUI_CANCEL),
        0,
        callback,
        callbackData
    };

    confirmationDialogShow(&savefileList->confirmationDialog, &dialogParams);
}

void savefileListConfirmLoad(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData) {
    struct ConfirmationDialogParams dialogParams = {
        translationsGet(GAMEUI_CONFIRMLOADGAME_TITLE),
        translationsGet(GAMEUI_LOADWARNING),
        translationsGet(GAMEUI_YES),
        translationsGet(GAMEUI_NO),
        0,
        callback,
        callbackData
    };

    confirmationDialogShow(&savefileList->confirmationDialog, &dialogParams);
}
