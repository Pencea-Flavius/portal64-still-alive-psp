#ifndef __MENU_PSP_MENU_BUILDER_RENDER_H__
#define __MENU_PSP_MENU_BUILDER_RENDER_H__

// The PSP half of the menu builder's drawing; see src/menu/n64/menu_builder_render.h.

#include "menu/menu_builder.h"

void textMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState);
void checkboxMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState);
void sliderMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState);

void menuBuilderRender(struct MenuBuilder* menuBuilder, struct RenderState* renderState);

#endif
