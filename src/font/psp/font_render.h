#ifndef __FONT_PSP_FONT_RENDER_H__
#define __FONT_PSP_FONT_RENDER_H__

// The PSP half of font drawing; see src/font/n64/font_render.h.
// A prerendered string keeps its symbols per font image, relative to its
// origin, and draws a sprite per symbol.

#include "font/font.h"
#include "graphics/color.h"
#include "graphics/psp/psp_render.h"

// What a prerendered string holds per font image. The N64 keeps a display list
// here instead.
struct PspTextImage {
    struct SymbolLocation* symbols;
    unsigned int color;
    short symbolCount;
};

// Draws a laid out string immediately, without keeping anything.
void fontRendererDraw(
    struct FontRenderer* renderer,
    const struct PspMaterial* const* fontImages,
    int x, int y,
    struct Coloru8* color,
    struct RenderState* renderState
);

void fontRendererInitPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender);
void fontRendererFillPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender, int x, int y, struct Coloru8* color);

struct PrerenderedText* prerenderedTextCopy(struct PrerenderedText* text);
void prerenderedTextCleanup(struct PrerenderedText* prerender);
void prerenderedTextRelocate(struct PrerenderedText* prerender, int x, int y);
void prerenderedTextRecolor(struct PrerenderedText* prerender, struct Coloru8* color);

void prerenderedBatchFinish(
    struct PrerenderedTextBatch* batch,
    const struct PspMaterial* const* fontImages,
    struct RenderState* renderState
);

#endif
