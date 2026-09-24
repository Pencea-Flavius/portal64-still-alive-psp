#ifndef __FONT_FONT_H__
#define __FONT_FONT_H__

#include "graphics/color.h"
#include "graphics/render_types.h"
#include "math/vector2s16.h"

#include "codegen/assets/strings/strings.h"

struct FontKerning {
    char amount;
    short first;
    short second;
};

struct FontSymbol {
    short id;
    char x, y;
    char width, height;
    char xoffset, yoffset;
    char xadvance;
    char textureIndex;
};

struct Font {
    struct FontKerning* kerning;
    struct FontSymbol* symbols;

    char base;
    char charHeight;
    unsigned short symbolMultiplier;
    unsigned short symbolMask;
    unsigned short symbolMaxCollisions;

    unsigned short kerningMultiplier;
    unsigned short kerningMask;
    unsigned short kerningMaxCollisions;
};

struct SymbolLocation {
    short x;
    short y;
    char sourceX;
    char sourceY;
    char width;
    char height;
    char canBreak;
    char imageIndex;
};

#define FONT_RENDERER_MAX_SYBMOLS   MAX_STRING_LENGTH

struct FontRenderer {
    struct SymbolLocation symbols[FONT_RENDERER_MAX_SYBMOLS];
    short currentSymbol;
    short width;
    short height;
    short usedImageIndices;
};

void fontRendererLayout(struct FontRenderer* renderer, struct Font* font, char* message, int maxWidth);

// One entry per font image, holding whatever that machine needs to draw the
// symbols that came out of the atlas: a display list on the N64, the symbols
// themselves on the PSP.
struct PrerenderedText {
    RenderDisplayList* displayLists;
    short usedImageIndices;
    short x;
    short y;
    short width;
    short height;
};

struct PrerenderedText* prerenderedTextNew(struct FontRenderer* renderer);
void prerenderedTextFree(struct PrerenderedText* prerender);

#define MAX_PRERENDERED_STRINGS     32

struct PrerenderedTextBatch {
    struct PrerenderedText* text[MAX_PRERENDERED_STRINGS];
    unsigned short textCount;
    short usedImageIndices;
};

struct PrerenderedTextBatch* prerenderedBatchStart();
void prerenderedBatchAdd(struct PrerenderedTextBatch* batch, struct PrerenderedText* text, struct Coloru8* color);

// Everything that draws is declared in the platform's half, which the build
// puts on the include path. It is included from here rather than from each of
// the thirteen files that call it, all of which want both halves anyway.
#include "font_render.h"

#endif