#ifndef __MENU_NEW_GAME_MENU_H__
#define __MENU_NEW_GAME_MENU_H__

#include "graphics/render_types.h"
#include "confirmation_dialog.h"
#include "font/font.h"
#include "graphics/render_types.h"
#include "menu.h"

#define CHAPTER_IMAGE_WIDTH     84
#define CHAPTER_IMAGE_HEIGHT    48

#define CHAPTER_IMAGE_SIZE      (CHAPTER_IMAGE_WIDTH * CHAPTER_IMAGE_HEIGHT * 2)

#define NEW_GAME_X          40
#define NEW_GAME_Y          45

// The frame around a chapter's screenshot; sizes are in
// new_game_menu_render.h.
#define CHAPTER_BORDER_WIDTH    (CHAPTER_DRAW_WIDTH + 8)
#define CHAPTER_BORDER_HEIGHT   (CHAPTER_DRAW_HEIGHT + 10)
#define CHAPTER_BORDER_PADDING  4

// Drawing is the platform's half; included here because a chapter item holds
// one of its types.
#include "new_game_menu_render.h"

struct Chapter {
    void* imageData;
    short testChamberLevelIndex;
    short testChamberNumber;
};

struct ChapterMenuItem {
    struct PrerenderedText* chapterText;
    struct PrerenderedText* testChamberText;
    RenderDisplayList border;
    void* imageBuffer;
    struct Chapter* chapter;
    // Index into gChapters; the PSP draws the screenshot from its material.
    int chapterIndex;
    struct ChapterMenuItemRender render;
    int x;
    int y;
};

struct NewGameMenu {
    struct PrerenderedText* newGameText;
    RenderDisplayList menuOutline;
    RenderDisplayList topLine;

    struct ConfirmationDialog confirmationDialog;

    struct ChapterMenuItem leftChapter;
    struct ChapterMenuItem rightChapter;

    short leftChapterIndex;
    short selectedChapterIndex;
    short chapterCount;
};

// Number of chapters, for both halves of the drawing.
extern const int gChapterCount;

void newGameInit(struct NewGameMenu* newGameMenu);
void newGameRebuildText(struct NewGameMenu* newGameMenu);
enum InputCapture newGameUpdate(struct NewGameMenu* newGameMenu);

#endif