#ifndef __MENU_PSP_NEW_GAME_MENU_RENDER_H__
#define __MENU_PSP_NEW_GAME_MENU_RENDER_H__

// The PSP half of the new game menu's drawing; see src/menu/n64/new_game_menu_render.h.
// Screenshots are drawn straight from their materials; nothing is copied.

#include "graphics/renderstate.h"

#ifndef CHAPTER_IMAGE_SIZE
#error "include menu/new_game_menu.h rather than this directly"
#endif

// Layout across the 400 wide box, with screenshots at twice the N64 size.
#define CHAPTER_DRAW_WIDTH      (CHAPTER_IMAGE_WIDTH * 2)
#define CHAPTER_DRAW_HEIGHT     (CHAPTER_IMAGE_HEIGHT * 2)
#define NEW_GAME_TITLE_X        (NEW_GAME_X + 12)
#define NEW_GAME_TITLE_Y        48
#define NEW_GAME_LINE_X         (NEW_GAME_X + 12)
#define NEW_GAME_LINE_Y         64
#define NEW_GAME_LINE_WIDTH     (SCREEN_WD - NEW_GAME_LINE_X * 2)
#define CHAPTER_GAP             24
#define CHAPTER_LEFT_X          ((SCREEN_WD - CHAPTER_BORDER_WIDTH * 2 - CHAPTER_GAP) / 2)
#define CHAPTER_RIGHT_X         (CHAPTER_LEFT_X + CHAPTER_BORDER_WIDTH + CHAPTER_GAP)
#define CHAPTER_Y               76

struct ChapterMenuItem;
struct NewGameMenu;

// C has no empty structure, hence the field.
struct ChapterMenuItemRender {
    char nothingRetained;
};

void chapterMenuItemRenderInit(struct ChapterMenuItem* chapterMenuItem);
void chapterMenuItemImageUpdate(struct ChapterMenuItem* chapterMenuItem);

void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task);

#endif
