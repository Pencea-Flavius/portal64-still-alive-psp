#include "menu_render.h"

#include "menu/menu.h"
#include "util/memory.h"

#include "codegen/assets/materials/ui.h"

// Nothing to build: widgets carry their rectangles.
RenderDisplayList menuBuildOutline(int x, int y, int width, int height, int invert) {
    (void)x; (void)y; (void)width; (void)height; (void)invert;
    return NULL;
}

void menuOutlineRelocate(RenderDisplayList outline, int x, int y, int width, int height, int invert) {
    (void)outline; (void)x; (void)y; (void)width; (void)height; (void)invert;
}

RenderDisplayList menuCheckboxBuildBack(int x, int y) {
    (void)x; (void)y;
    return NULL;
}

RenderDisplayList menuSliderBuildBack(int x, int y, int w, int tickCount) {
    (void)x; (void)y; (void)w; (void)tickCount;
    return NULL;
}

void menuOutlineDraw(struct RenderState* renderState, int x, int y, int width, int height, int invert) {
    unsigned int top = pspRenderColor(invert ? &gBorderDark : &gBorderHighlight);
    unsigned int bottom = pspRenderColor(invert ? &gBorderHighlight : &gBorderDark);

    pspRenderFillRect(renderState, x, y, width - 1, 1, top);
    pspRenderFillRect(renderState, x, y, 1, height, top);
    pspRenderFillRect(renderState, x, y + height - 1, width, 1, bottom);
    pspRenderFillRect(renderState, x + width - 1, y, 1, height - 1, bottom);
}

void menuButtonDraw(struct RenderState* renderState, struct MenuButton* button) {
    menuOutlineDraw(renderState, button->x, button->y, button->w, button->h, 0);
}

void menuCheckboxDraw(struct RenderState* renderState, struct MenuCheckbox* checkbox) {
    struct Coloru8 back = {93, 96, 97, 255};

    pspRenderFillRect(renderState, checkbox->x, checkbox->y, CHECKBOX_SIZE, CHECKBOX_SIZE, pspRenderColor(&back));
    menuOutlineDraw(renderState, checkbox->x, checkbox->y, CHECKBOX_SIZE, CHECKBOX_SIZE, 1);

    if (checkbox->checked) {
        pspRenderFillRect(renderState, checkbox->x + 3, checkbox->y + 3, 5, 5, 0xFFFFFFFF);
    }
}

void menuSliderDraw(struct RenderState* renderState, struct MenuSlider* slider) {
    struct Coloru8 trackColor = {25, 25, 25, 255};
    struct Coloru8 handleColor = {93, 96, 97, 255};

    int sliderY = slider->y + (SLIDER_HEIGHT / 2) - (SLIDER_TRACK_HEIGHT / 2);

    pspRenderFillRect(renderState, slider->x, sliderY, slider->w, SLIDER_TRACK_HEIGHT, pspRenderColor(&trackColor));

    int tickMin = slider->x + (SLIDER_WIDTH / 2);
    int tickWidth = slider->w - SLIDER_WIDTH;

    for (int i = 0; i < slider->tickCount; ++i) {
        int tickX = slider->tickCount <= 1 ? tickMin : (i * tickWidth) / (slider->tickCount - 1) + tickMin;

        pspRenderFillRect(renderState, tickX, slider->y + TICK_Y, 1, TICK_HEIGHT, pspRenderColor(&trackColor));
    }

    menuOutlineDraw(renderState, slider->x, sliderY, slider->w, SLIDER_TRACK_HEIGHT, 1);

    int sliderPos = (slider->w - SLIDER_WIDTH) * slider->value + slider->x + (SLIDER_WIDTH / 2);

    pspRenderFillRect(
        renderState,
        sliderPos - (SLIDER_WIDTH / 2), slider->y,
        SLIDER_WIDTH, SLIDER_HEIGHT,
        pspRenderColor(&handleColor)
    );
    menuOutlineDraw(renderState, sliderPos - (SLIDER_WIDTH / 2), slider->y, SLIDER_WIDTH, SLIDER_HEIGHT, 0);
}

void tabsOutlineInit(struct Tabs* tabs) {
    tabs->tabOutline = NULL;
}

void tabsOutlineRender(struct Tabs* tabs) {
    // tabsOutlineDraw() reads the layout directly.
    (void)tabs;
}

void tabsOutlineDraw(struct Tabs* tabs, struct RenderState* renderState) {
    unsigned int dark = pspRenderColor(&gBorderDark);
    unsigned int highlight = pspRenderColor(&gBorderHighlight);

    int tabOffset = tabs->prevOffset;

    pspRenderFillRect(renderState, tabs->x, tabs->y + tabs->height - 1, tabs->width, 1, dark);
    pspRenderFillRect(renderState, tabs->x + tabs->width - 1, tabs->y + TAB_HEIGHT, 1, tabs->height - TAB_HEIGHT, dark);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);
        int tabLeft = tab->x + tabOffset;

        pspRenderFillRect(renderState, tabLeft + tab->width - 2, tabTop, 1, tabs->y + TAB_HEIGHT - tabTop, dark);
    }

    pspRenderFillRect(renderState, tabs->x, tabs->y + TAB_HEIGHT, 1, tabs->height - TAB_HEIGHT, highlight);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);
        int tabLeft = tab->x + tabOffset;

        pspRenderFillRect(renderState, tabLeft, tabTop, 1, tabs->y + TAB_HEIGHT - tabTop, highlight);
        pspRenderFillRect(renderState, tabLeft, tabTop, tab->width - 2, 1, highlight);
    }

    struct TabRenderData* selectedTab = tabs->selectedTab < tabs->tabCount ? &tabs->tabRenderData[tabs->selectedTab] : NULL;

    if (selectedTab) {
        int selectedLeft = selectedTab->x + tabOffset;

        pspRenderFillRect(renderState, tabs->x, tabs->y + TAB_HEIGHT, selectedLeft - tabs->x, 1, highlight);
        pspRenderFillRect(
            renderState,
            selectedLeft + selectedTab->width,
            tabs->y + TAB_HEIGHT,
            (tabs->x + tabs->width) - (selectedLeft + selectedTab->width),
            1,
            highlight
        );
    }
}

RenderDisplayList menuBuildBorder(int x, int y, int width, int height) {
    struct MenuBorder* border = malloc(sizeof(struct MenuBorder));

    menuBorderRelocate(border, x, y, width, height);

    return border;
}

void menuBorderRelocate(RenderDisplayList handle, int x, int y, int width, int height) {
    struct MenuBorder* border = handle;

    if (!border) {
        return;
    }

    border->x = (short)x;
    border->y = (short)y;
    border->width = (short)width;
    border->height = (short)height;
}

// Seven rectangles from the rounded corner atlas, as on the N64: four
// corners and three stretched spans, one bind.
void menuBorderDraw(struct RenderState* renderState, RenderDisplayList handle, unsigned int color) {
    struct MenuBorder* border = handle;

    if (!border) {
        return;
    }

    const struct PspMaterial* material = ui_material_list[ROUNDED_CORNERS_INDEX];

    if (!material->texture) {
        return;
    }

    int x = border->x;
    int y = border->y;
    int w = border->width;
    int h = border->height;

    // The corner is the top left four texels; the middle repeats (4, 4).
    struct { int x, y, w, h; int u0, v0, u1, v1; } spans[] = {
        {x,         y,         4,     4,     0, 0, 4, 4},
        {x + 4,     y,         w - 8, 4,     4, 4, 4, 4},
        {x + w - 4, y,         4,     4,     4, 0, 0, 4},
        {x,         y + 4,     w,     h - 8, 4, 4, 4, 4},
        {x,         y + h - 4, 4,     4,     0, 4, 4, 0},
        {x + 4,     y + h - 4, w - 8, 4,     4, 4, 4, 4},
        {x + w - 4, y + h - 4, 4,     4,     4, 4, 0, 0},
    };

    for (unsigned i = 0; i < sizeof(spans) / sizeof(*spans); ++i) {
        pspRenderTextureRect(
            renderState, material,
            spans[i].x, spans[i].y, spans[i].w, spans[i].h,
            spans[i].u0, spans[i].v0, spans[i].u1, spans[i].v1,
            color
        );
    }
}

RenderDisplayList menuBuildHorizontalLine(int x, int y, int width) {
    struct MenuBorder* line = malloc(sizeof(struct MenuBorder));

    menuBorderRelocate(line, x, y, width, 2);

    return line;
}

void menuHorizontalLineDraw(struct RenderState* renderState, RenderDisplayList handle) {
    struct MenuBorder* line = handle;

    if (!line) {
        return;
    }

    struct Coloru8 dark = {86, 86, 86, 128};
    struct Coloru8 light = {193, 193, 193, 128};

    pspRenderFillRect(renderState, line->x, line->y, line->width, 1, pspRenderColor(&dark));
    pspRenderFillRect(renderState, line->x, line->y + 1, line->width, 1, pspRenderColor(&light));
}

RenderDisplayList menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh) {
    struct MenuSolidBorder* border = malloc(sizeof(struct MenuSolidBorder));

    menuSolidBorderRelocate(border, x, y, w, h, nx, ny, nw, nh);

    return border;
}

void menuSolidBorderRelocate(RenderDisplayList handle, int x, int y, int w, int h, int nx, int ny, int nw, int nh) {
    struct MenuSolidBorder* border = handle;

    if (!border) {
        return;
    }

    border->x = (short)x;
    border->y = (short)y;
    border->w = (short)w;
    border->h = (short)h;
    border->nx = (short)nx;
    border->ny = (short)ny;
    border->nw = (short)nw;
    border->nh = (short)nh;
}

// Four fills around the hole, the same four the N64 builds.
void menuSolidBorderDraw(struct RenderState* renderState, RenderDisplayList handle, unsigned int color) {
    struct MenuSolidBorder* border = handle;

    if (!border) {
        return;
    }

    pspRenderFillRect(renderState, border->x, border->y, border->w, border->ny - border->y, color);
    pspRenderFillRect(renderState, border->x, border->ny, border->nx - border->x, border->nh, color);
    pspRenderFillRect(renderState, border->nx + border->nw, border->ny, (border->x + border->w) - (border->nx + border->nw), border->nh, color);
    pspRenderFillRect(renderState, border->x, border->ny + border->nh, border->w, (border->y + border->h) - (border->ny + border->nh), color);
}

struct Coloru8* menuRenderColor(int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor) {
    return isSelected ? selected : defaultColor;
}
