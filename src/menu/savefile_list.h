#ifndef __MENU_SAVEFILE_LIST_H__
#define __MENU_SAVEFILE_LIST_H__

#include "confirmation_dialog.h"
#include "font/font.h"
#include "graphics/render_types.h"
#include "menu.h"
#include "new_game_menu.h"
#include "savefile/savefile.h"
#include "system/display.h"

// Thumbnail, frame and list geometry, for both halves.
#define SAVE_SLOT_RENDER_W  (SAVE_SLOT_IMAGE_W * 2)
#define SAVE_SLOT_RENDER_H  (SAVE_SLOT_IMAGE_H * 2)

#define BORDER_THICKNESS    5
#define BORDER_WIDTH        (SAVE_SLOT_RENDER_W + BORDER_THICKNESS * 2)
#define BORDER_HEIGHT       (SAVE_SLOT_RENDER_H + BORDER_THICKNESS * 2)

#define ROW_HEIGHT          (BORDER_HEIGHT + 8)

#define LOAD_GAME_LEFT       40
#define LOAD_GAME_TOP        15

#define FILE_OFFSET_X        16
#define FILE_OFFSET_Y        28

// The row of control hints along the bottom.
#define CONTROL_HINTS_HEIGHT 10
#define CONTROL_TEXT_PADDING 12
#define CONTROL_TEXT_MARGIN  2

#define CONTENT_X       (LOAD_GAME_LEFT + 8)
#define CONTENT_Y       (LOAD_GAME_TOP + FILE_OFFSET_Y - 8)
#define CONTENT_WIDTH   (SCREEN_WD - CONTENT_X * 2)
#define CONTENT_HEIGHT  (SCREEN_HT - CONTENT_Y - LOAD_GAME_TOP - CONTROL_HINTS_HEIGHT - 8)

#define MAX_VISIBLE_SLOTS  4

// Drawing is the platform's half; included here because a slot holds one of
// its types.
#include "savefile_list_render.h"

struct SavefileInfo {
    short slotIndex;
    short testChamberNumber;
    short testSubjectNumber;
    char* savefileName;
    int isFree;
};

struct SavefileListSlot {
    struct PrerenderedText* testChamberText;
    RenderDisplayList border;
    struct PrerenderedText* gameId;
    short x, y;
    short slotIndex;
    void* imageData;
    struct SavefileListSlotRender render;
};

struct SavefileListMenu {
    RenderDisplayList menuOutline;
    struct PrerenderedText* savefileListTitleText;
    struct PrerenderedText* deleteText;
    struct PrerenderedText* confirmText;
    struct ConfirmationDialog confirmationDialog;
    struct SavefileInfo savefileInfo[MAX_SAVE_SLOTS];
    struct SavefileListSlot slots[MAX_VISIBLE_SLOTS];
    short numberOfSaves;
    short scrollOffset;
    short indexOffset;
    short selectedSave;
};

void savefileListMenuInit(struct SavefileListMenu* savefileList);
void savefileUseList(struct SavefileListMenu* savefileList, char* title, char* confirmLabel, struct SavefileInfo* savefileInfo, int slotCount);
enum InputCapture savefileListUpdate(struct SavefileListMenu* savefileList);
int savefileGetSlot(struct SavefileListMenu* savefileList);
void savefileListConfirmDeletion(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData);
void savefileListConfirmOverwrite(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData);
void savefileListConfirmLoad(struct SavefileListMenu* savefileList, ConfirmationDialogCallback callback, void* callbackData);

#endif