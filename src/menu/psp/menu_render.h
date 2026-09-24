#ifndef __MENU_PSP_MENU_RENDER_H__
#define __MENU_PSP_MENU_RENDER_H__

// The PSP half of menu drawing; see src/menu/n64/menu_render.h. Drawing is
// immediate, so nothing is retained.

#include "graphics/psp/psp_render.h"
#include "menu/menu.h"
#include "menu/tabs.h"

// The shared widget builders; they return nothing here.
RenderDisplayList menuBuildOutline(int x, int y, int width, int height, int invert);
void menuOutlineRelocate(RenderDisplayList outline, int x, int y, int width, int height, int invert);
RenderDisplayList menuCheckboxBuildBack(int x, int y);
RenderDisplayList menuSliderBuildBack(int x, int y, int w, int tickCount);

void menuOutlineDraw(struct RenderState* renderState, int x, int y, int width, int height, int invert);
void menuButtonDraw(struct RenderState* renderState, struct MenuButton* button);
void menuCheckboxDraw(struct RenderState* renderState, struct MenuCheckbox* checkbox);
void menuSliderDraw(struct RenderState* renderState, struct MenuSlider* slider);

// A retained border is just its rectangle.
struct MenuBorder {
    short x, y;
    short width, height;
};

// A solid border: an outer rectangle minus an inner one.
struct MenuSolidBorder {
    short x, y, w, h;
    short nx, ny, nw, nh;
};

RenderDisplayList menuBuildBorder(int x, int y, int width, int height);
RenderDisplayList menuBuildHorizontalLine(int x, int y, int width);
RenderDisplayList menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh);

void menuBorderRelocate(RenderDisplayList border, int x, int y, int width, int height);
void menuSolidBorderRelocate(RenderDisplayList border, int x, int y, int w, int h, int nx, int ny, int nw, int nh);

// The border's colour is passed to the draw (an env colour on the N64).
void menuBorderDraw(struct RenderState* renderState, RenderDisplayList border, unsigned int color);
void menuHorizontalLineDraw(struct RenderState* renderState, RenderDisplayList line);
void menuSolidBorderDraw(struct RenderState* renderState, RenderDisplayList border, unsigned int color);

// Each draw takes its colour; this only picks it.
struct Coloru8* menuRenderColor(int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor);

// Nothing is retained, so this only clears the field.
void tabsOutlineInit(struct Tabs* tabs);

// Nothing is built ahead of time, so a layout change needs no rebuild.
void tabsOutlineRender(struct Tabs* tabs);

// Draws the outline for the layout currently in the struct.
void tabsOutlineDraw(struct Tabs* tabs, struct RenderState* renderState);

#endif
