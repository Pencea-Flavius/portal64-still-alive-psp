#include "menu/new_game_menu.h"

#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "savefile/savefile.h"
#include "graphics/psp/psp_render.h"
#include "system/display.h"

#include "codegen/assets/materials/images.h"

#include <pspgu.h>

static struct Coloru8 gOverlayColor = {0, 0, 0, 85};
static struct Coloru8 gMenuBorderColor = {164, 164, 164, 128};

// gChapters' order, which is chapter number, against the material list's,
// which is the alphabetical order the generator emits. They disagree from ten
// onwards, so the mapping is written out rather than computed.
static const int sChapterMaterial[] = {
    CHAPTER1_INDEX,
    CHAPTER2_INDEX,
    CHAPTER3_INDEX,
    CHAPTER4_INDEX,
    CHAPTER5_INDEX,
    CHAPTER6_INDEX,
    CHAPTER7_INDEX,
    CHAPTER8_INDEX,
    CHAPTER9_INDEX,
    CHAPTER10_INDEX,
    CHAPTER11_INDEX,
};

void chapterMenuItemRenderInit(struct ChapterMenuItem* chapterMenuItem) {
    // Nothing to build: the screenshot is a material the pipeline emitted.
    (void)chapterMenuItem;
}

void chapterMenuItemImageUpdate(struct ChapterMenuItem* chapterMenuItem) {
    // Nothing to upload, for the same reason.
    (void)chapterMenuItem;
}

static void chapterMenuItemDrawImage(struct ChapterMenuItem* chapterMenuItem, struct RenderState* renderState) {
    int index = chapterMenuItem->chapterIndex;

    if (index < 0 || index >= (int)(sizeof(sChapterMaterial) / sizeof(*sChapterMaterial))) {
        return;
    }

    // The texture is padded up to a power of two, so the screenshot is the top
    // left corner of it and the coordinates stop at its own size.
    pspRenderTextureRect(
        renderState, images_material_list[sChapterMaterial[index]],
        chapterMenuItem->x + CHAPTER_BORDER_PADDING,
        chapterMenuItem->testChamberText->y + chapterMenuItem->testChamberText->height + 9,
        CHAPTER_DRAW_WIDTH, CHAPTER_DRAW_HEIGHT,
        0, 0, CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
        pspRenderColor(&gColorWhite)
    );
}

void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    (void)task;

    pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&gOverlayColor));

    menuBorderDraw(renderState, newGameMenu->menuOutline, pspRenderColor(&gMenuBorderColor));
    menuHorizontalLineDraw(renderState, newGameMenu->topLine);

    int leftChapterSelected = newGameMenu->selectedChapterIndex == newGameMenu->leftChapterIndex;
    int showRightChapter = (newGameMenu->leftChapterIndex + 1) < gChapterCount &&
        newGameMenu->rightChapter.chapter->testChamberLevelIndex <= gSaveData.header.chapterProgressLevelIndex &&
        newGameMenu->rightChapter.chapter->testChamberLevelIndex > 0;

    menuSolidBorderDraw(
        renderState,
        newGameMenu->leftChapter.border,
        pspRenderColor(menuRenderColor(leftChapterSelected, &gSelectionOrange, &gColorBlack))
    );

    if (showRightChapter) {
        menuSolidBorderDraw(
            renderState,
            newGameMenu->rightChapter.border,
            pspRenderColor(menuRenderColor(!leftChapterSelected, &gSelectionOrange, &gColorBlack))
        );
    }

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    prerenderedBatchAdd(batch, newGameMenu->newGameText, NULL);

    prerenderedBatchAdd(batch, newGameMenu->leftChapter.chapterText, leftChapterSelected ? &gSelectionOrange : &gColorWhite);
    prerenderedBatchAdd(batch, newGameMenu->leftChapter.testChamberText, leftChapterSelected ? &gSelectionOrange : &gColorWhite);

    if (showRightChapter) {
        prerenderedBatchAdd(batch, newGameMenu->rightChapter.chapterText, !leftChapterSelected ? &gSelectionOrange : &gColorWhite);
        prerenderedBatchAdd(batch, newGameMenu->rightChapter.testChamberText, !leftChapterSelected ? &gSelectionOrange : &gColorWhite);
    }

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);

    chapterMenuItemDrawImage(&newGameMenu->leftChapter, renderState);

    if (showRightChapter) {
        chapterMenuItemDrawImage(&newGameMenu->rightChapter, renderState);
    }

    if (newGameMenu->confirmationDialog.isShown) {
        confirmationDialogRender(&newGameMenu->confirmationDialog, renderState);
    }
}
