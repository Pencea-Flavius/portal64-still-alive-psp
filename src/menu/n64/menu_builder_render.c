#include "menu_builder_render.h"

#include "font/dejavu_sans.h"
#include "menu/menu.h"

#include "codegen/assets/materials/ui.h"

// The row geometry the selection highlight is drawn against. Deliberately not
// in menu_builder.h: four menus define their own MENU_WIDTH.
#define MENU_WIDTH 252
#define TEXTHEIGHT 12
#define PADDING_X 2
#define TABWIDTH 232

void textMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    if (textBatch) {
        prerenderedBatchAdd(textBatch, element->data, selection == element->selectionIndex ? &gColorBlack : &gColorWhite);
        int isTextPositionedOnFarLeft = (element->params->x < (int)(MENU_WIDTH / 2));

        if ((selection == element->selectionIndex) && isTextPositionedOnFarLeft){
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
            gDPFillRectangle(
                renderState->dl++, 
                element->params->x - PADDING_X, 
                element->params->y, 
                element->params->x + TABWIDTH + PADDING_X, 
                element->params->y + TEXTHEIGHT
            );
        }
    }
}

void checkboxMenuItemRender(struct MenuBuilderElement* element, 
    int selection, 
    int materialIndex, 
    struct PrerenderedTextBatch* textBatch, 
    struct RenderState* renderState) {
    struct MenuCheckbox* checkbox = (struct MenuCheckbox*)element->data;

    if (materialIndex == SOLID_ENV_INDEX) {
        gSPDisplayList(renderState->dl++, checkbox->outline);
        renderState->dl = menuCheckboxRender(checkbox, renderState->dl);
    } else if (textBatch) {
        prerenderedBatchAdd(textBatch, checkbox->prerenderedText, selection == element->selectionIndex ? &gColorBlack : &gColorWhite);

        if (selection == element->selectionIndex) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
            gDPFillRectangle(
                renderState->dl++, 
                element->params->x + CHECKBOX_SIZE + 4, 
                element->params->y, 
                element->params->x + MENU_WIDTH - CHECKBOX_SIZE - 4,
                element->params->y + CHECKBOX_SIZE
            );
        }
    }
}

void sliderMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    struct MenuSlider* slider = (struct MenuSlider*)element->data;
    gSPDisplayList(renderState->dl++, slider->back);
    renderState->dl = menuSliderRender(slider, renderState->dl);
}

void menuBuilderRender(struct MenuBuilder* menuBuilder, struct RenderState* renderState) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        menuBuilder->elements[i].callbacks->render(&menuBuilder->elements[i], menuBuilder->selection, SOLID_ENV_INDEX, NULL, renderState);
    }
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);


    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        menuBuilder->elements[i].callbacks->render(&menuBuilder->elements[i], menuBuilder->selection, -1, batch, renderState);
    }
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);
}
