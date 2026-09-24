#include "menu_render.h"

#include "menu/menu.h"
#include "util/memory.h"

Gfx* menuRerenderBorder(int x, int y, int width, int height, Gfx* dl) {
    gSPTextureRectangle(
        dl++,
        x << 2, y << 2,
        (x + 4) << 2, (y + 4) << 2,
        G_TX_RENDERTILE,
        0 << 5, 0 << 5,
        0x400, 0x400
    );

    gSPTextureRectangle(
        dl++,
        (x + 4) << 2, y << 2,
        (x + width - 4) << 2, (y + 4) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0, 0
    );

    gSPTextureRectangle(
        dl++,
        (x + width - 4) << 2, y << 2,
        (x + width) << 2, (y + 4) << 2,
        G_TX_RENDERTILE,
        4 << 5, 0 << 5,
        0x400, 0x400
    );

    gSPTextureRectangle(
        dl++,
        x << 2, (y + 4) << 2,
        (x + width) << 2, (y + height - 4) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0, 0
    );

    gSPTextureRectangle(
        dl++,
        x << 2, (y + height - 4) << 2,
        (x + 4) << 2, (y + height) << 2,
        G_TX_RENDERTILE,
        0 << 5, 4 << 5,
        0x400, 0x400
    );

    gSPTextureRectangle(
        dl++,
        (x + 4) << 2, (y + height - 4) << 2,
        (x + width - 4) << 2, (y + height) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0, 0
    );

    gSPTextureRectangle(
        dl++,
        (x + width - 4) << 2, (y + height - 4) << 2,
        (x + width) << 2, (y + height) << 2,
        G_TX_RENDERTILE,
        4 << 5, 4 << 5,
        0x400, 0x400
    );

    return dl;
}

Gfx* menuBuildBorder(int x, int y, int width, int height) {
    Gfx* result = malloc(sizeof(Gfx) * 7 * 3 + 1);
    Gfx* dl = menuRerenderBorder(x, y, width, height, result);

    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuBuildHorizontalLine(int x, int y, int width) {
    Gfx* result = malloc(sizeof(Gfx) * 7);

    Gfx* dl = result;
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 86, 86, 86, 128);
    gDPFillRectangle(dl++, x, y, x + width, y + 1);
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 193, 193, 193, 128);
    gDPFillRectangle(dl++, x, y + 1, x + width, y + 2);
    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuRerenderSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh, Gfx* dl) {
    gDPFillRectangle(dl++, x, y, x + w, ny);
    gDPFillRectangle(dl++, x, ny, nx, ny + nh);
    gDPFillRectangle(dl++, nx + nw, ny, x + w, ny + nh);
    gDPFillRectangle(dl++, x, ny + nh, x + w, y + h);
    return dl;
}

// The neutral shape of menuRerenderSolidBorder(): the shared layout moves a
// border without knowing whether that rewrites commands or eight fields.
void menuSolidBorderRelocate(RenderDisplayList border, int x, int y, int w, int h, int nx, int ny, int nw, int nh) {
    menuRerenderSolidBorder(x, y, w, h, nx, ny, nw, nh, border);
}

Gfx* menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh) {
    Gfx* result = malloc(sizeof(Gfx) * 5);
    Gfx* dl = menuRerenderSolidBorder(x, y, w, h, nx, ny, nw, nh, result);

    gSPEndDisplayList(dl++);

    return result;
}

Gfx* menuRenderOutline(int x, int y, int width, int height, int invert, Gfx* dl) {
    gDPPipeSync(dl++);
    if (invert) {
        gDPSetEnvColor(dl++, gBorderDark.r, gBorderDark.g, gBorderDark.b, gBorderDark.a);
    } else {
        gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    }
    gDPFillRectangle(dl++, x, y, x + width - 1, y + 1);
    gDPFillRectangle(dl++, x, y, x + 1, y + height);
    gDPPipeSync(dl++);
    if (invert) {
        gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    } else {
        gDPSetEnvColor(dl++, gBorderDark.r, gBorderDark.g, gBorderDark.b, gBorderDark.a);
    }
    gDPFillRectangle(dl++, x, y + height - 1, x + width, y + height);
    gDPFillRectangle(dl++, x + width - 1, y, x + width, y + height - 1);

    return dl;
}

Gfx* menuBuildOutline(int x, int y, int width, int height, int invert) {
    Gfx* result = malloc(sizeof(Gfx) * 9);
    Gfx* dl = menuRenderOutline(x, y, width, height, invert, result);
    gSPEndDisplayList(dl++);
    return result;
}

void menuSetRenderColor(struct RenderState* renderState, int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor) {
    if (isSelected) {
        gDPSetEnvColor(renderState->dl++, selected->r, selected->g, selected->b, selected->a);
    } else {
        gDPSetEnvColor(renderState->dl++, defaultColor->r, defaultColor->g, defaultColor->b, defaultColor->a);
    }
}

Gfx* menuCheckboxRender(struct MenuCheckbox* checkbox, Gfx* dl) {
    if (!checkbox->checked) {
        return dl;
    }

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 255, 255, 255, 255);
    gDPFillRectangle(
        dl++, 
        checkbox->x + 3, 
        checkbox->y + 3, 
        checkbox->x + 8, 
        checkbox->y + 8
    );
    return dl;
}

Gfx* menuSliderRender(struct MenuSlider* slider, Gfx* dl) {
    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 93, 96, 97, 255);

    int sliderPos = (slider->w - SLIDER_WIDTH) * slider->value + slider->x + (SLIDER_WIDTH / 2);

    gDPFillRectangle(
        dl++, 
        sliderPos - (SLIDER_WIDTH / 2), 
        slider->y, 
        sliderPos + (SLIDER_WIDTH / 2), 
        slider->y + SLIDER_HEIGHT
    );
    dl = menuRenderOutline(sliderPos - (SLIDER_WIDTH / 2), slider->y, SLIDER_WIDTH, SLIDER_HEIGHT, 0, dl);

    return dl;
}

// The widget builders in menu.c own these; here they are a malloc and a
// display list.
void menuOutlineRelocate(RenderDisplayList outline, int x, int y, int width, int height, int invert) {
    menuRenderOutline(x, y, width, height, invert, outline);
}

void menuBorderRelocate(RenderDisplayList border, int x, int y, int width, int height) {
    menuRerenderBorder(x, y, width, height, border);
}

RenderDisplayList menuCheckboxBuildBack(int x, int y) {
    Gfx* result = malloc(sizeof(Gfx) * 12);

    Gfx* dl = result;

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 93, 96, 97, 255);
    gDPFillRectangle(dl++, x, y, x + CHECKBOX_SIZE, y + CHECKBOX_SIZE);
    dl = menuRenderOutline(x, y, CHECKBOX_SIZE, CHECKBOX_SIZE, 1, dl);
    gSPEndDisplayList(dl++);

    return result;
}

RenderDisplayList menuSliderBuildBack(int x, int y, int w, int tickCount) {
    Gfx* result = malloc(sizeof(Gfx) * (12 + tickCount));

    Gfx* dl = result;

    int sliderX = x;
    int sliderY = y + (SLIDER_HEIGHT / 2) - (SLIDER_TRACK_HEIGHT / 2);

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, 25, 25, 25, 255);
    gDPFillRectangle(
        dl++, 
        sliderX, 
        sliderY, 
        sliderX + w, 
        sliderY + SLIDER_TRACK_HEIGHT
    );
    
    int tickMin = x + (SLIDER_WIDTH / 2);
    int tickWidth = w - SLIDER_WIDTH;
    for (int i = 0; i < tickCount; ++i) {
        int tickX = tickCount <= 1 ? tickMin : (i * tickWidth) / (tickCount - 1) + tickMin;
        gDPFillRectangle(
            dl++, 
            tickX, 
            y + TICK_Y, 
            tickX + 1, 
            y + TICK_Y + TICK_HEIGHT
        );  
    }

    dl = menuRenderOutline(sliderX, sliderY, w, SLIDER_TRACK_HEIGHT, 1, dl);

    gSPEndDisplayList(dl++);

    return result;
}

void tabsOutlineInit(struct Tabs* tabs) {
    tabs->tabOutline = malloc(sizeof(Gfx) * (10 + 3 * tabs->tabCount));
}

void tabsOutlineRender(struct Tabs* tabs) {
    Gfx* dl = tabs->tabOutline;

    int tabOffset = tabs->prevOffset;

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gBorderDark.r, gBorderDark.g, gBorderDark.b, gBorderDark.a);
    gDPFillRectangle(dl++, tabs->x, tabs->y + tabs->height - 1, tabs->x + tabs->width, tabs->y + tabs->height);
    gDPFillRectangle(dl++, tabs->x + tabs->width - 1, tabs->y + TAB_HEIGHT, tabs->x + tabs->width, tabs->y + tabs->height);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);
        int tabLeft = tab->x + tabOffset;

        gDPFillRectangle(dl++, tabLeft + tab->width - 2, tabTop, tabLeft + tab->width - 1, tabs->y + TAB_HEIGHT);
    }

    gDPPipeSync(dl++);
    gDPSetEnvColor(dl++, gBorderHighlight.r, gBorderHighlight.g, gBorderHighlight.b, gBorderHighlight.a);
    gDPFillRectangle(dl++, tabs->x, tabs->y + TAB_HEIGHT, tabs->x + 1, tabs->y + tabs->height);

    for (int i = 0; i < tabs->tabCount; ++i) {
        struct TabRenderData* tab = &tabs->tabRenderData[i];
        int tabTop = (i == tabs->selectedTab) ? tabs->y : (tabs->y + 1);
        int tabLeft = tab->x + tabOffset;

        gDPFillRectangle(dl++, tabLeft, tabTop, tabLeft + 1, tabs->y + TAB_HEIGHT);
        gDPFillRectangle(dl++, tabLeft, tabTop, tabLeft + tab->width - 2, tabTop + 1);
    }

    struct TabRenderData* selectedTab = tabs->selectedTab < tabs->tabCount ? &tabs->tabRenderData[tabs->selectedTab] : NULL;

    if (selectedTab) {
        gDPFillRectangle(dl++, tabs->x, tabs->y + TAB_HEIGHT, selectedTab->x + tabOffset, tabs->y + TAB_HEIGHT + 1);
        gDPFillRectangle(dl++, selectedTab->x + tabOffset + selectedTab->width, tabs->y + TAB_HEIGHT, tabs->x + tabs->width, tabs->y + TAB_HEIGHT + 1);
    }

    gSPEndDisplayList(dl++);
}
