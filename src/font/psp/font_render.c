#include "font_render.h"

#include "util/memory.h"

// The number of image indices a mask reaches.
static int fontImageCount(int imageMask) {
    int count = 0;

    while (imageMask) {
        imageMask >>= 1;
        ++count;
    }

    return count;
}

static int fontSymbolsInImage(struct FontRenderer* renderer, int imageIndex) {
    int count = 0;

    for (int i = 0; i < renderer->currentSymbol; ++i) {
        if (renderer->symbols[i].imageIndex == imageIndex) {
            ++count;
        }
    }

    return count;
}

// Source rectangles are in texels, as 2D sprites want.
static void fontAddSymbol(struct PspSpriteBatch* batch, struct SymbolLocation* symbol, int x, int y, unsigned int color) {
    pspRenderSpriteBatchAdd(
        batch,
        x + symbol->x, y + symbol->y,
        symbol->width, symbol->height,
        symbol->sourceX,
        symbol->sourceY,
        symbol->sourceX + symbol->width,
        symbol->sourceY + symbol->height,
        color
    );
}

// Every symbol on one atlas image in one draw.
void fontRendererDraw(
    struct FontRenderer* renderer,
    const struct PspMaterial* const* fontImages,
    int x, int y,
    struct Coloru8* color,
    struct RenderState* renderState
) {
    unsigned int drawColor = color ? pspRenderColor(color) : 0xFFFFFFFF;
    int imageCount = fontImageCount(renderer->usedImageIndices);

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        int count = fontSymbolsInImage(renderer, imageIndex);
        struct PspSpriteBatch batch;

        if (!count || !pspRenderSpriteBatchBegin(&batch, renderState, fontImages[imageIndex], count)) {
            continue;
        }

        for (int i = 0; i < renderer->currentSymbol; ++i) {
            struct SymbolLocation* symbol = &renderer->symbols[i];

            if (symbol->imageIndex == imageIndex) {
                fontAddSymbol(&batch, symbol, x, y, drawColor);
            }
        }

        pspRenderSpriteBatchEnd(&batch);
    }
}

void fontRendererInitPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender) {
    int imageCount = fontImageCount(renderer->usedImageIndices);

    prerender->displayLists = malloc(sizeof(RenderDisplayList) * imageCount);

    prerender->usedImageIndices = renderer->usedImageIndices;
    prerender->x = 0;
    prerender->y = 0;

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        int symbolCount = (renderer->usedImageIndices & (1 << imageIndex))
            ? fontSymbolsInImage(renderer, imageIndex)
            : 0;

        if (!symbolCount) {
            prerender->displayLists[imageIndex] = NULL;
            continue;
        }

        struct PspTextImage* image = malloc(sizeof(struct PspTextImage));

        image->symbols = malloc(sizeof(struct SymbolLocation) * symbolCount);
        image->symbolCount = symbolCount;
        image->color = 0xFFFFFFFF;

        prerender->displayLists[imageIndex] = image;
    }
}

void fontRendererFillPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender, int x, int y, struct Coloru8* color) {
    int imageCount = fontImageCount(renderer->usedImageIndices & prerender->usedImageIndices);

    prerender->x = x;
    prerender->y = y;
    prerender->width = renderer->width;
    prerender->height = renderer->height;

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        struct PspTextImage* image = prerender->displayLists[imageIndex];

        if (!image) {
            continue;
        }

        image->color = color ? pspRenderColor(color) : 0xFFFFFFFF;

        int used = 0;

        for (int i = 0; i < renderer->currentSymbol && used < image->symbolCount; ++i) {
            if (renderer->symbols[i].imageIndex != imageIndex) {
                continue;
            }

            image->symbols[used] = renderer->symbols[i];
            ++used;
        }

        image->symbolCount = used;
    }
}

struct PrerenderedText* prerenderedTextCopy(struct PrerenderedText* text) {
    struct PrerenderedText* result = malloc(sizeof(struct PrerenderedText));

    int imageCount = fontImageCount(text->usedImageIndices);

    result->displayLists = malloc(sizeof(RenderDisplayList) * imageCount);
    result->usedImageIndices = text->usedImageIndices;
    result->x = text->x;
    result->y = text->y;
    result->width = text->width;
    result->height = text->height;

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        struct PspTextImage* source = text->displayLists[imageIndex];

        if (!source) {
            result->displayLists[imageIndex] = NULL;
            continue;
        }

        struct PspTextImage* image = malloc(sizeof(struct PspTextImage));
        int size = sizeof(struct SymbolLocation) * source->symbolCount;

        image->symbols = malloc(size);
        image->symbolCount = source->symbolCount;
        image->color = source->color;

        for (int i = 0; i < source->symbolCount; ++i) {
            image->symbols[i] = source->symbols[i];
        }

        result->displayLists[imageIndex] = image;
    }

    return result;
}

void prerenderedTextCleanup(struct PrerenderedText* prerender) {
    int imageCount = fontImageCount(prerender->usedImageIndices);

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        struct PspTextImage* image = prerender->displayLists[imageIndex];

        if (!image) {
            continue;
        }

        free(image->symbols);
        free(image);
    }

    free(prerender->displayLists);
}

// Symbols are relative to the string's origin, so moving it is one store.
void prerenderedTextRelocate(struct PrerenderedText* prerender, int x, int y) {
    prerender->x = x;
    prerender->y = y;
}

void prerenderedTextRecolor(struct PrerenderedText* prerender, struct Coloru8* color) {
    int imageCount = fontImageCount(prerender->usedImageIndices);
    unsigned int drawColor = color ? pspRenderColor(color) : 0xFFFFFFFF;

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        struct PspTextImage* image = prerender->displayLists[imageIndex];

        if (image) {
            image->color = drawColor;
        }
    }
}

// Grouped by image, one bind each.
void prerenderedBatchFinish(
    struct PrerenderedTextBatch* batch,
    const struct PspMaterial* const* fontImages,
    struct RenderState* renderState
) {
    int imageCount = fontImageCount(batch->usedImageIndices);

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        if (!(batch->usedImageIndices & (1 << imageIndex))) {
            continue;
        }

        int count = 0;

        for (int i = 0; i < batch->textCount; ++i) {
            struct PrerenderedText* text = batch->text[i];
            struct PspTextImage* image = text->displayLists[imageIndex];

            if ((text->usedImageIndices & (1 << imageIndex)) && image) {
                count += image->symbolCount;
            }
        }

        struct PspSpriteBatch sprites;

        if (!count || !pspRenderSpriteBatchBegin(&sprites, renderState, fontImages[imageIndex], count)) {
            continue;
        }

        for (int i = 0; i < batch->textCount; ++i) {
            struct PrerenderedText* text = batch->text[i];
            struct PspTextImage* image = text->displayLists[imageIndex];

            if (!(text->usedImageIndices & (1 << imageIndex)) || !image) {
                continue;
            }

            for (int symbol = 0; symbol < image->symbolCount; ++symbol) {
                fontAddSymbol(&sprites, &image->symbols[symbol], text->x, text->y, image->color);
            }
        }

        pspRenderSpriteBatchEnd(&sprites);
    }

    stackMallocFree(batch);
}
