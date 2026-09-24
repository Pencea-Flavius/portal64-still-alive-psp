#ifndef __MENU_N64_MENU_RENDER_H__
#define __MENU_N64_MENU_RENDER_H__

// The N64 half of menu drawing (src/menu/psp/menu_render.h): display lists
// built ahead of time and kept in the menu structures.

#include <ultra64.h>

#include "graphics/color.h"
#include "graphics/renderstate.h"
#include "menu/menu.h"
#include "menu/tabs.h"

// Display list entries a font image and a terminator take.
#define GFX_ENTRIES_PER_IMAGE   3
#define GFX_ENTRIES_PER_END_DL  1

Gfx* menuRerenderBorder(int x, int y, int width, int height, Gfx* dl);
Gfx* menuBuildBorder(int x, int y, int width, int height);
Gfx* menuBuildHorizontalLine(int x, int y, int width);
Gfx* menuRerenderSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh, Gfx* dl);
Gfx* menuBuildSolidBorder(int x, int y, int w, int h, int nx, int ny, int nw, int nh);
Gfx* menuRenderOutline(int x, int y, int width, int height, int invert, Gfx* dl);
Gfx* menuBuildOutline(int x, int y, int width, int height, int invert);

// Called by the shared widget builders; moving a widget rewrites its
// retained outline.
void menuOutlineRelocate(RenderDisplayList outline, int x, int y, int width, int height, int invert);
void menuBorderRelocate(RenderDisplayList border, int x, int y, int width, int height);
void menuSolidBorderRelocate(RenderDisplayList border, int x, int y, int w, int h, int nx, int ny, int nw, int nh);
RenderDisplayList menuCheckboxBuildBack(int x, int y);
RenderDisplayList menuSliderBuildBack(int x, int y, int w, int tickCount);

Gfx* menuCheckboxRender(struct MenuCheckbox* checkbox, Gfx* dl);
Gfx* menuSliderRender(struct MenuSlider* slider, Gfx* dl);

void menuSetRenderColor(struct RenderState* renderState, int isSelected, struct Coloru8* selected, struct Coloru8* defaultColor);

// Allocates the display list the tab outline is built into.
void tabsOutlineInit(struct Tabs* tabs);

// Rebuilds it from the layout in the struct.
void tabsOutlineRender(struct Tabs* tabs);

#endif
