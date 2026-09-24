#include "menu.h"

#include "system/display.h"
#include "util/memory.h"

struct Coloru8 gSelectionOrange = {255, 156, 0, 255};
struct Coloru8 gSelectionGray = {201, 201, 201, 255};
struct Coloru8 gBorderHighlight = {193, 193, 193, 255};
struct Coloru8 gBorderDark = {86, 86, 86, 255};

struct PrerenderedText* menuBuildPrerenderedText(struct Font* font, char* message, int x, int y, int maxWidth) {
    struct FontRenderer* renderer = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(renderer, font, message, maxWidth);
    struct PrerenderedText* result = prerenderedTextNew(renderer);
    fontRendererFillPrerender(renderer, result, x, y, NULL);
    stackMallocFree(renderer);
    return result;
}

struct MenuButton menuBuildButton(struct Font* font, char* message, int x, int y, int height, int rightAlign) {
    struct MenuButton result;

    result.text = menuBuildPrerenderedText(font, message, x + BUTTON_LEFT_PADDING, y + BUTTON_TOP_PADDING, SCREEN_HT);

    int width = result.text->width + BUTTON_LEFT_PADDING + BUTTON_RIGHT_PADDING;

    if (rightAlign) {
        x -= width;
        prerenderedTextRelocate(result.text, x + BUTTON_LEFT_PADDING, y + BUTTON_TOP_PADDING);
    }

    result.outline = menuBuildOutline(x, y, width, height, 0);

    result.x = x;
    result.y = y;

    result.w = width;
    result.h = height;

    return result;
}

void menuRebuildButtonText(struct MenuButton* button, struct Font* font, char* message, int rightAlign) {
    menuFreePrerenderedDeferred(button->text);

    button->text = menuBuildPrerenderedText(font, message, button->x + BUTTON_LEFT_PADDING, button->y + BUTTON_TOP_PADDING, SCREEN_HT);

    int newWidth = button->text->width + BUTTON_LEFT_PADDING + BUTTON_RIGHT_PADDING;

    if (rightAlign) {
        button->x -= newWidth - button->w;
        prerenderedTextRelocate(button->text, button->x + BUTTON_LEFT_PADDING, button->y + BUTTON_TOP_PADDING);
    }

    button->w = newWidth;

    menuOutlineRelocate(button->outline, button->x, button->y, button->w, button->h, 0);
}

void menuRelocateButton(struct MenuButton* button, int x, int y, int rightAlign) {
    if (rightAlign) {
        x -= button->w;
    }

    menuOutlineRelocate(button->outline, x, y, button->w, button->h, 0);
    prerenderedTextRelocate(button->text, x + BUTTON_LEFT_PADDING, y + BUTTON_TOP_PADDING);

    button->x = x;
    button->y = y;
}

struct MenuCheckbox menuBuildCheckbox(struct Font* font, char* message, int x, int y) {
    struct MenuCheckbox result;

    result.x = x;
    result.y = y;

    result.outline = menuCheckboxBuildBack(x, y);

    result.prerenderedText = menuBuildPrerenderedText(font, message, x + CHECKBOX_SIZE + 6, y, SCREEN_WD);
    result.checked = 0;

    return result;
}

struct MenuSlider menuBuildSlider(int x, int y, int w, int tickCount) {
    struct MenuSlider result;

    result.x = x;
    result.y = y;
    result.w = w;

    result.back = menuSliderBuildBack(x, y, w, tickCount);
    result.tickCount = tickCount;

    result.value = 0;

    return result;
}

#define MAX_DEFERRED_RELEASE_SIZE   20
#define RELEASE_DEFER_COUNT         2
#define NEXT_ENTRY(curr)        ((curr) + 1 == MAX_DEFERRED_RELEASE_SIZE ? 0 : (curr) + 1)

struct PrerenderedTextReleaseQueue {
    struct PrerenderedText* queue[MAX_DEFERRED_RELEASE_SIZE];
    unsigned char entryDelay[MAX_DEFERRED_RELEASE_SIZE];
    short insertPos;
    short readPos;
};

struct PrerenderedTextReleaseQueue gDeferredPTRelease;

void menuFreePrerenderedDeferred(struct PrerenderedText* text) {
    if (!text) {
        return;
    }

    if (gDeferredPTRelease.insertPos == gDeferredPTRelease.readPos && gDeferredPTRelease.entryDelay[gDeferredPTRelease.readPos] != 0) {
        // queue full, we just leak memory now
        return;
    }

    gDeferredPTRelease.queue[gDeferredPTRelease.insertPos] = text;
    gDeferredPTRelease.entryDelay[gDeferredPTRelease.insertPos] = RELEASE_DEFER_COUNT;
    gDeferredPTRelease.insertPos = NEXT_ENTRY(gDeferredPTRelease.insertPos);
}

void menuTickDeferredQueue() {
    int curr = gDeferredPTRelease.readPos;

    if (gDeferredPTRelease.entryDelay[curr] == 0) {
        return;
    }

    do {
        --gDeferredPTRelease.entryDelay[curr];

        int next = NEXT_ENTRY(curr);

        if (gDeferredPTRelease.entryDelay[curr] == 0) {
            prerenderedTextFree(gDeferredPTRelease.queue[curr]);
            gDeferredPTRelease.queue[curr] = NULL;
            gDeferredPTRelease.readPos = next;
        }

        curr = next;
    } while (curr != gDeferredPTRelease.insertPos);
}

void menuResetDeferredQueue() {
    for (int i = 0; i < MAX_DEFERRED_RELEASE_SIZE; ++i) {
        gDeferredPTRelease.queue[i] = NULL;
        gDeferredPTRelease.entryDelay[i] = 0;
    }
    gDeferredPTRelease.insertPos = 0;
    gDeferredPTRelease.readPos = 0;
}