#include "levels/credits.h"

#include "font/font.h"
#include "font/liberation_mono.h"
#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "system/display.h"
#include "util/memory.h"

#include "codegen/assets/materials/ui.h"

// Positions are the N64's 320x240 layout, unchanged.
static void creditsDrawText(struct FontRenderer* renderer, const char* text, int x, int y, struct Coloru8* color, struct RenderState* renderState) {
    fontRendererLayout(renderer, &gLiberationMonoFont, (char*)text, SCREEN_WD);
    fontRendererDraw(renderer, gLiberationMonoImages, x, y, color, renderState);
}

static void creditsDrawImage(struct RenderState* renderState, const struct PspMaterial* material, int x, int y, int size) {
    if (!material->texture) {
        return;
    }

    pspRenderTextureRect(
        renderState, material,
        x, y, size, size,
        0, 0, material->texture->width, material->texture->height,
        0xFFFFFFFF
    );
}

void creditsRender(void* data, struct RenderState* renderState, struct GraphicsTask* task) {
    (void)task;

    struct Credits* credits = (struct Credits*)data;

    struct Coloru8 black = {0, 0, 0, 255};
    pspRenderFillRect(renderState, 0, 0, SCREEN_WD, SCREEN_HT, pspRenderColor(&black));

    struct Coloru8 color;
    creditsTextColor(credits, &color);

    struct FontRenderer* renderer = stackMalloc(sizeof(struct FontRenderer));

    creditsDrawText(renderer, "THANK YOU FOR PARTICIPATING\nIN THIS\nENRICHMENT CENTER ACTIVITY!!\n\nIt is still in development.", 35, 36, &color, renderState);

    creditsDrawText(renderer, "-----------------------------------------", 14, 12, &color, renderState);
    creditsDrawText(renderer, "-----------------------------------------", 14, SCREEN_HT - 24, &color, renderState);

    creditsDrawText(renderer, "|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|", 14, 24, &color, renderState);
    creditsDrawText(renderer, "|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|", 294, 24, &color, renderState);

    creditsDrawText(renderer, "GitHub", 74, 120, &color, renderState);

    creditsDrawImage(renderState, ui_material_list[GITHUB_QR_INDEX], 74, 138, 64);
    creditsDrawImage(renderState, ui_material_list[CREDITS_ICONS_INDEX], 34, 134, 32);

    stackMallocFree(renderer);
}
