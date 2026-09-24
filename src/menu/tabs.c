#include "tabs.h"

#include "menu.h"
#include "menu_render.h"
#include "strings/translations.h"
#include "system/display.h"
#include "util/memory.h"

#define LEFT_TEXT_PADDING  4
#define RIGHT_TEXT_PADDING 16
#define TOP_TEXT_PADDING   3

void tabsSetSelectedTab(struct Tabs* tabs, int index) {
    if (index < 0 || index >= tabs->tabCount) {
        return;
    }

    tabs->selectedTab = index;

    int tabOffset = 0;

    struct TabRenderData* rightVisibleTab = &tabs->tabRenderData[tabs->selectedTab];

    if (tabs->selectedTab + 1 < tabs->tabCount) {
        ++rightVisibleTab;
    }

    if (rightVisibleTab->x + rightVisibleTab->width >= tabs->x + tabs->width) {
        tabOffset = (tabs->x + tabs->width) - (rightVisibleTab->x + rightVisibleTab->width) + 1;
    }

    tabs->prevOffset = tabOffset;

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);

        prerenderedTextRelocate(tab->text, tab->x + tabOffset + LEFT_TEXT_PADDING, tabTop + TOP_TEXT_PADDING);
    }

    tabsOutlineRender(tabs);
}

void tabsInit(struct Tabs* tabs, struct Tab* tabList, int tabCount, struct Font* font, int x, int y, int width, int height) {
    tabs->tabs = tabList;
    tabs->tabCount = tabCount;
    tabs->font = font;
    tabs->width = width;
    tabs->height = height;
    tabs->x = x;
    tabs->y = y;
    tabs->prevOffset = 0;
    tabs->tabRenderData = malloc(sizeof(struct TabRenderData) * tabCount);

    for (int i = 0; i < tabCount; ++i) {
        tabs->tabRenderData[i].text = NULL;
    }

    tabsOutlineInit(tabs);

    tabs->selectedTab = 0;
    tabsRebuildText(tabs);
}

void tabsRenderText(struct Tabs* tabs, struct PrerenderedTextBatch* batch) {
    for (int i = 0; i < tabs->tabCount; ++i) {
        prerenderedBatchAdd(batch, tabs->tabRenderData[i].text, i == tabs->selectedTab ? &gColorWhite : &gSelectionGray);
    }
}

void tabsRebuildText(struct Tabs* tabs) {
    int currentX = tabs->x;

    for (int i = 0; i < tabs->tabCount; ++i) {
        prerenderedTextFree(tabs->tabRenderData[i].text);
        tabs->tabRenderData[i].text = menuBuildPrerenderedText(
            tabs->font, 
            translationsGet(tabs->tabs[i].messageId), 
            currentX + LEFT_TEXT_PADDING, 
            tabs->y + TOP_TEXT_PADDING,
            SCREEN_WD
        );
        tabs->tabRenderData[i].width = tabs->tabRenderData[i].text->width + LEFT_TEXT_PADDING + RIGHT_TEXT_PADDING;
        tabs->tabRenderData[i].x = currentX;

        currentX += tabs->tabRenderData[i].width;
    }

    int selectedTab = tabs->selectedTab;
    tabs->selectedTab = -1;
    tabsSetSelectedTab(tabs, selectedTab);
}