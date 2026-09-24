#include "font.h"

#include "math/mathf.h"
#include "util/memory.h"

#define TEXTURE_IMAGE_INDEX_TO_MASK(index) (1 << (index))

int fontDetermineKerning(struct Font* font, short first, short second) {
    unsigned index = ((unsigned)first * (unsigned)font->kerningMultiplier + (unsigned)second) & (unsigned)font->kerningMask;
    int maxIterations = font->kerningMaxCollisions;

    do {
        struct FontKerning* kerning = &font->kerning[index];

        if (kerning->amount == 0) {
            return 0;
        }

        if (kerning->first == first && kerning->second == second) {
            return kerning->amount;
        }
        
        ++index;
        --maxIterations;
    } while (maxIterations >= 0);

    return 0;
}

struct FontSymbol* fontFindSymbol(struct Font* font, short id) {
    unsigned index = ((unsigned)id * (unsigned)font->symbolMultiplier) & (unsigned)font->symbolMask;
    int maxIterations = font->symbolMaxCollisions;

    do {
        struct FontSymbol* symbol = &font->symbols[index];

        if (symbol->textureIndex == -1) {
            return NULL;
        }

        if (symbol->id == id) {
            return symbol;
        }
        
        ++index;
        --maxIterations;
    } while (maxIterations >= 0);

    return NULL;
}

short fontNextUtf8Character(char** strPtr) {
    char* curr = *strPtr;

    // in the middle of a code point
    // try to find the start of a charcter
    while ((*curr & 0xC0) == 0x80) {
        ++curr;
    }

    if (!(*curr & 0x80)) {
        *strPtr = curr + 1;
        return *curr;
    }

    if ((*curr & 0xE0) == 0xC0) {
        *strPtr = curr + 2;
        return ((short)(curr[0] & 0x1F) << 6) | (short)(curr[1] & 0x3F);

    } else if ((*curr & 0xF0) == 0xE0) {
        *strPtr = curr + 3;
        return ((short)(curr[0] & 0xF) << 12) | ((short)(curr[1] & 0x3F) << 6) | (short)(curr[2] & 0x3F);

    } else if ((*curr & 0xF8) == 0xF0) {
        *strPtr = curr + 4;
        // utf character out of range of a short
        return 0;
    } else {
        // invalid unicode character
        *strPtr = curr + 1;
        return 0;
    }
}

int fontRendererFindBreak(struct FontRenderer* renderer) {
    for (int search = renderer->currentSymbol - 1; search > 0; --search) {
        if (renderer->symbols[search].canBreak) {
            return search + 1;
        }
    }

    return renderer->currentSymbol - 1;
}

void fontRendererWrap(struct FontRenderer* renderer, int from, int xOffset, int yOffset) {
    for (int i = from; i < renderer->currentSymbol; ++i) {
        renderer->symbols[i].x += xOffset;
        renderer->symbols[i].y += yOffset;
    }
}

void fontRendererLayout(struct FontRenderer* renderer, struct Font* font, char* message, int maxWidth) {
    renderer->width = 0;
    renderer->height = 0;
    renderer->currentSymbol = 0;
    renderer->usedImageIndices = 0;

    short prev = 0;
    short curr = 0;
    int x = 0;
    int y = 0;
    int currentMaxWidth = 0;

    while (*message && renderer->currentSymbol < FONT_RENDERER_MAX_SYBMOLS) {
        prev = curr;
        // also advances message to the next character
        curr = fontNextUtf8Character(&message);

        if (curr == '\n') {
            currentMaxWidth = MAX(currentMaxWidth, x);
            y += font->charHeight;
            x = 0;
            continue;
        }

        struct FontSymbol* symbol = fontFindSymbol(font, curr);

        if (!symbol) {
            continue;
        }

        x += fontDetermineKerning(font, prev, curr);

        struct SymbolLocation* target = &renderer->symbols[renderer->currentSymbol];

        target->x = x + symbol->xoffset;
        target->y = y + symbol->yoffset;
        target->width = symbol->width;
        target->height = symbol->height;
        target->canBreak = curr == ' ';
        target->sourceX = symbol->x;
        target->sourceY = symbol->y;
        target->imageIndex = symbol->textureIndex;

        renderer->usedImageIndices |= TEXTURE_IMAGE_INDEX_TO_MASK(symbol->textureIndex);

        ++renderer->currentSymbol;
        x += symbol->xadvance;

        if (x > maxWidth) {
            int breakAt = fontRendererFindBreak(renderer);

            if (breakAt == renderer->currentSymbol) {
                currentMaxWidth = MAX(currentMaxWidth, x);
                y += font->charHeight;
                x = 0;
            } else {
                int lastCharacterX = renderer->symbols[breakAt].x;
                currentMaxWidth = MAX(currentMaxWidth, lastCharacterX);
                fontRendererWrap(renderer, breakAt, -lastCharacterX, font->charHeight);
                y += font->charHeight;
                x -= lastCharacterX;
            }
        }
    }

    renderer->width = MAX(currentMaxWidth, x);
    renderer->height = y + font->charHeight;
}

struct PrerenderedText* prerenderedTextNew(struct FontRenderer* renderer) {
    struct PrerenderedText* result = malloc(sizeof(struct PrerenderedText*));
    fontRendererInitPrerender(renderer, result);
    return result;
}

void prerenderedTextFree(struct PrerenderedText* prerender) {
    if (!prerender) {
        return;
    }

    prerenderedTextCleanup(prerender);
    free(prerender);
}

struct PrerenderedTextBatch* prerenderedBatchStart() {
    struct PrerenderedTextBatch* result = stackMalloc(sizeof(struct PrerenderedTextBatch));
    result->textCount = 0;
    result->usedImageIndices = 0;
    return result;
}

void prerenderedBatchAdd(struct PrerenderedTextBatch* batch, struct PrerenderedText* text, struct Coloru8* color) {
    if (batch->textCount >= MAX_PRERENDERED_STRINGS) {
        return;
    }
    prerenderedTextRecolor(text, color);
    batch->text[batch->textCount] = text;
    ++batch->textCount;
    batch->usedImageIndices |= text->usedImageIndices;
}
