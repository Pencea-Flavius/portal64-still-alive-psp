#include "menu/new_game_menu.h"

#include "font/dejavu_sans.h"
#include "image.h"
#include "savefile/savefile.h"
#include "system/display.h"

#include "codegen/assets/materials/ui.h"

// The tile is loaded from the shared buffer each frame, so there is nothing to
// set up and nothing to refresh when that buffer changes.
void chapterMenuItemRenderInit(struct ChapterMenuItem* chapterMenuItem) {
    (void)chapterMenuItem;
}

void chapterMenuItemImageUpdate(struct ChapterMenuItem* chapterMenuItem) {
    (void)chapterMenuItem;
}

void newGameRender(struct NewGameMenu* newGameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[DEFAULT_UI_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, 0, 0, SCREEN_WD, SCREEN_HT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[ROUNDED_CORNERS_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->menuOutline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[ROUNDED_CORNERS_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, newGameMenu->topLine);

    int leftChapterSelected = newGameMenu->selectedChapterIndex == newGameMenu->leftChapterIndex;
    int showRightChapter = (newGameMenu->leftChapterIndex + 1) < gChapterCount &&
        newGameMenu->rightChapter.chapter->testChamberLevelIndex <= gSaveData.header.chapterProgressLevelIndex &&
        newGameMenu->rightChapter.chapter->testChamberLevelIndex > 0;

    gDPPipeSync(renderState->dl++);
    menuSetRenderColor(renderState, leftChapterSelected, &gSelectionOrange, &gColorBlack);
    gSPDisplayList(renderState->dl++, newGameMenu->leftChapter.border);

    if (showRightChapter) {
        gDPPipeSync(renderState->dl++);
        menuSetRenderColor(renderState, !leftChapterSelected, &gSelectionOrange, &gColorBlack);
        gSPDisplayList(renderState->dl++, newGameMenu->rightChapter.border);
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    prerenderedBatchAdd(batch, newGameMenu->newGameText, NULL);

    prerenderedBatchAdd(batch, newGameMenu->leftChapter.chapterText, leftChapterSelected ? &gSelectionOrange : &gColorWhite);
    prerenderedBatchAdd(batch, newGameMenu->leftChapter.testChamberText, leftChapterSelected ? &gSelectionOrange : &gColorWhite);

    if (showRightChapter) {
        prerenderedBatchAdd(batch, newGameMenu->rightChapter.chapterText, !leftChapterSelected ? &gSelectionOrange : &gColorWhite);
        prerenderedBatchAdd(batch, newGameMenu->rightChapter.testChamberText, !leftChapterSelected ? &gSelectionOrange : &gColorWhite);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    graphicsCopyImage(
        renderState, newGameMenu->leftChapter.imageBuffer,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
        0, 0,
        CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
        newGameMenu->leftChapter.x + 4,
        newGameMenu->leftChapter.testChamberText->y + newGameMenu->leftChapter.testChamberText->height + 9,
        gColorWhite
    );

    if (showRightChapter) {
        graphicsCopyImage(
            renderState, newGameMenu->rightChapter.imageBuffer,
            CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
            0, 0,
            CHAPTER_IMAGE_WIDTH, CHAPTER_IMAGE_HEIGHT,
            newGameMenu->rightChapter.x + 4,
            newGameMenu->rightChapter.testChamberText->y + newGameMenu->rightChapter.testChamberText->height + 9,
            gColorWhite
        );
    }

    if (newGameMenu->confirmationDialog.isShown) {
        confirmationDialogRender(&newGameMenu->confirmationDialog, renderState);
    }
}
