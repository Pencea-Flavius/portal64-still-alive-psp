#ifndef __FONT_N64_FONT_RENDER_H__
#define __FONT_N64_FONT_RENDER_H__

// The N64 half of font drawing (src/font/psp/...): prerendered text is a
// display list per font image, patched in place when moved or recoloured.

#include <ultra64.h>

#include "font/font.h"
#include "graphics/color.h"

// Draws a laid out string straight into a display list.
Gfx* fontRendererBuildGfx(struct FontRenderer* renderer, Gfx** fontImages, int x, int y, struct Coloru8* color, Gfx* gfx);

// Allocates the per-image display lists a prerendered string needs.
void fontRendererInitPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender);
void fontRendererFillPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender, int x, int y, struct Coloru8* color);

struct PrerenderedText* prerenderedTextCopy(struct PrerenderedText* text);
void prerenderedTextCleanup(struct PrerenderedText* prerender);
void prerenderedTextRelocate(struct PrerenderedText* prerender, int x, int y);
void prerenderedTextRecolor(struct PrerenderedText* prerender, struct Coloru8* color);

Gfx* prerenderedBatchFinish(struct PrerenderedTextBatch* batch, Gfx** fontImages, Gfx* gfx);

#endif
