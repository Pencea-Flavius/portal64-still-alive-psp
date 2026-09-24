#ifndef __MENU_PSP_SAVEFILE_LIST_RENDER_H__
#define __MENU_PSP_SAVEFILE_LIST_RENDER_H__

// The PSP half of the savefile list's drawing; see src/menu/n64/savefile_list_render.h.
// Each slot keeps a PspImage, since the GE only samples power of two
// textures.

#include "graphics/psp/psp_image.h"
#include "graphics/renderstate.h"

#ifndef MAX_VISIBLE_SLOTS
#error "include menu/savefile_list.h rather than this directly"
#endif

struct SavefileListSlot;
struct SavefileListMenu;

struct SavefileListSlotRender {
    struct PspImage image;
};

void savefileListSlotRenderInit(struct SavefileListSlot* slot);
void savefileListSlotImageUpdate(struct SavefileListSlot* slot);

void savefileListRender(struct SavefileListMenu* savefileList, struct RenderState* renderState, struct GraphicsTask* task);

#endif
