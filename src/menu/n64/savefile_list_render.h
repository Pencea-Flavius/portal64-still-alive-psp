#ifndef __MENU_N64_SAVEFILE_LIST_RENDER_H__
#define __MENU_N64_SAVEFILE_LIST_RENDER_H__

// The N64 half of the savefile list's drawing; see src/menu/psp/savefile_list_render.h.
// Thumbnails are drawn from the shared copy.

#include "graphics/renderstate.h"

#ifndef MAX_VISIBLE_SLOTS
#error "include menu/savefile_list.h rather than this directly"
#endif

struct SavefileListSlot;
struct SavefileListMenu;

// C has no empty structure, hence the field.
struct SavefileListSlotRender {
    char nothingRetained;
};

void savefileListSlotRenderInit(struct SavefileListSlot* slot);
void savefileListSlotImageUpdate(struct SavefileListSlot* slot);

void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task);

#endif
