#ifndef __MENU_N64_NEW_GAME_MENU_RENDER_H__
#define __MENU_N64_NEW_GAME_MENU_RENDER_H__

// The N64 half of the new game menu's drawing; see src/menu/psp/new_game_menu_render.h.
// Screenshots are drawn from the shared buffer.

#include "graphics/renderstate.h"

#ifndef CHAPTER_IMAGE_SIZE
#error "include menu/new_game_menu.h rather than this directly"
#endif

// Layout on the N64's 320x240.
#define CHAPTER_DRAW_WIDTH      CHAPTER_IMAGE_WIDTH
#define CHAPTER_DRAW_HEIGHT     CHAPTER_IMAGE_HEIGHT
#define NEW_GAME_TITLE_X        48
#define NEW_GAME_TITLE_Y        48
#define NEW_GAME_LINE_X         52
#define NEW_GAME_LINE_Y         64
#define NEW_GAME_LINE_WIDTH     214
#define CHAPTER_LEFT_X          55
#define CHAPTER_RIGHT_X         163
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
