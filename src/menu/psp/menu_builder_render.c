#include "menu_builder_render.h"

#include "font/dejavu_sans.h"
#include "graphics/psp/psp_model_render.h"
#include "menu/menu.h"

#include "codegen/assets/materials/ui.h"

// The row geometry the selection highlight is drawn against. Deliberately not
// in menu_builder.h: four menus define their own MENU_WIDTH.
#define MENU_WIDTH 252
#define TEXTHEIGHT 12
#define PADDING_X 2
#define TABWIDTH 232

// The N64 fills these through the display list against an env colour it has
// just set; here the colour rides along with the rectangle.
static void menuBuilderHighlight(struct RenderState* renderState, int x, int y, int width, int height) {
    pspRenderFillRect(renderState, x, y, width, height, pspRenderColor(&gSelectionOrange));
}

void textMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    (void)materialIndex;

    if (!textBatch) {
        return;
    }

    prerenderedBatchAdd(textBatch, element->data, selection == element->selectionIndex ? &gColorBlack : &gColorWhite);

    int isTextPositionedOnFarLeft = (element->params->x < (int)(MENU_WIDTH / 2));

    if ((selection == element->selectionIndex) && isTextPositionedOnFarLeft) {
        menuBuilderHighlight(
            renderState,
            element->params->x - PADDING_X,
            element->params->y,
            TABWIDTH + PADDING_X * 2,
            TEXTHEIGHT
        );
    }
}

void checkboxMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    struct MenuCheckbox* checkbox = (struct MenuCheckbox*)element->data;

    // The N64 splits this in two passes, one per material. Here the checkbox
    // and its highlight are both plain rectangles, so the material index only
    // picks which pass draws what.
    if (materialIndex != -1) {
        menuCheckboxDraw(renderState, checkbox);
        return;
    }

    if (!textBatch) {
        return;
    }

    prerenderedBatchAdd(textBatch, checkbox->prerenderedText, selection == element->selectionIndex ? &gColorBlack : &gColorWhite);

    if (selection == element->selectionIndex) {
        menuBuilderHighlight(
            renderState,
            element->params->x + CHECKBOX_SIZE + 4,
            element->params->y,
            MENU_WIDTH - (CHECKBOX_SIZE + 4) * 2,
            CHECKBOX_SIZE
        );
    }
}

void sliderMenuItemRender(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState) {
    (void)selection; (void)materialIndex; (void)textBatch;

    menuSliderDraw(renderState, (struct MenuSlider*)element->data);
}

// The N64 makes two passes, one per material, because a display list carries
// its state with it. Here the state is set once before each pass and the GU
// keeps it, so the shape is the same and there is nothing to revert.
void menuBuilderRender(struct MenuBuilder* menuBuilder, struct RenderState* renderState) {
    pspMaterialBind(ui_material_list[SOLID_ENV_INDEX]);

    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        menuBuilder->elements[i].callbacks->render(
            &menuBuilder->elements[i], menuBuilder->selection, SOLID_ENV_INDEX, NULL, renderState);
    }

    struct PrerenderedTextBatch* batch = prerenderedBatchStart();

    for (int i = 0; i < menuBuilder->elementCount; ++i) {
        menuBuilder->elements[i].callbacks->render(
            &menuBuilder->elements[i], menuBuilder->selection, -1, batch, renderState);
    }

    prerenderedBatchFinish(batch, gDejaVuSansImages, renderState);
}
