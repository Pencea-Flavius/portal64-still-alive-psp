#include "font_render.h"

#include "util/memory.h"

Gfx* fontRendererBuildSingleGfx(struct FontRenderer* renderer, int imageIndex, int x, int y, Gfx* gfx) {
    for (int i = 0; i < renderer->currentSymbol; ++i) {
        struct SymbolLocation* target = &renderer->symbols[i];

        if (target->imageIndex != imageIndex) {
            continue;
        }

        int finalX = target->x + x;
        int finalY = target->y + y;

        gSPTextureRectangle(
            gfx++, 
            finalX << 2, finalY << 2,
            (finalX + target->width) << 2,
            (finalY + target->height) << 2,
            G_TX_RENDERTILE,
            target->sourceX << 5, target->sourceY << 5,
            0x400, 0x400
        );
    }

    return gfx;
}

Gfx* fontRendererBuildGfx(struct FontRenderer* renderer, Gfx** fontImages, int x, int y, struct Coloru8* color, Gfx* gfx) {
    int imageMask = renderer->usedImageIndices;
    int imageIndex = 0;

    while (imageMask) {
        if (imageMask & 0x1) {
            gSPDisplayList(gfx++, fontImages[imageIndex]);

            if (color) {
                gDPSetEnvColor(gfx++, color->r, color->g, color->b, color->a);
            }

            gfx = fontRendererBuildSingleGfx(renderer, imageIndex, x, y, gfx);
        }

        imageMask >>= 1;
        ++imageIndex;
    }

    return gfx;
}

void fontRendererInitPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender) {
    int imageIndex = 0;
    int imageMask = renderer->usedImageIndices;

    while (imageMask) {
        imageMask >>= 1;
        ++imageIndex;
    }

    prerender->displayLists = malloc(sizeof(Gfx*) * imageIndex);

    prerender->usedImageIndices = renderer->usedImageIndices;
    prerender->x = 0;
    prerender->y = 0;

    imageMask = renderer->usedImageIndices;
    imageIndex = 0;

    while (imageMask) {
        if (imageMask & 0x1) {
            int symbolCount = 0;

            for (int i = 0; i < renderer->currentSymbol; ++i) {
                struct SymbolLocation* target = &renderer->symbols[i];

                if (target->imageIndex != imageIndex) {
                    continue;
                }

                ++symbolCount;
            }

            if (symbolCount) {
                // 3 gfx per symbol, + 2 for color change + 1 for end display list
                prerender->displayLists[imageIndex] = malloc(sizeof(Gfx) * (symbolCount * 3 + 3));
            } else {
                prerender->displayLists[imageIndex] = NULL;
            }
        } else {
            prerender->displayLists[imageIndex] = NULL;
        }

        imageMask >>= 1;
        ++imageIndex;
    }
}

struct PrerenderedText* prerenderedTextCopy(struct PrerenderedText* text) {
    struct PrerenderedText* result = malloc(sizeof(struct PrerenderedText));

    int imageIndex = 0;
    int imageMask = text->usedImageIndices;

    while (imageMask) {
        imageMask >>= 1;
        ++imageIndex;
    }

    result->displayLists = malloc(sizeof(Gfx*) * imageIndex);
    result->usedImageIndices = text->usedImageIndices;
    result->x = text->x;
    result->y = text->y;
    result->width = text->width;
    result->height = text->height;

    imageIndex = 0;
    imageMask = text->usedImageIndices;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* src = text->displayLists[imageIndex];

            src += 2;
            while (_SHIFTR(src->words.w0, 24, 8) != G_ENDDL) {
                // copy image
                src += 3;
            }
            ++src;

            int size = (src - text->displayLists[imageIndex]) * sizeof(Gfx);

            result->displayLists[imageIndex] = malloc(size);

            Gfx* dest = result->displayLists[imageIndex];
            src = text->displayLists[imageIndex];
            // copy color 
            *dest++ = *src++;
            *dest++ = *src++;

            while (_SHIFTR(src->words.w0, 24, 8) != G_ENDDL) {
                // copy image
                *dest++ = *src++;
                *dest++ = *src++;
                *dest++ = *src++;
            }

            // copy end
            *dest++ = *src++;

            osWritebackDCache(result->displayLists[imageIndex], size);
        } else {
            result->displayLists[imageIndex] = NULL;
        }

        imageMask >>= 1;
        ++imageIndex;
    }
    return result;
}

void prerenderedTextCleanup(struct PrerenderedText* prerender) {
    int imageIndex = 0;
    int imageMask = prerender->usedImageIndices;

    while (imageMask) {
        free(prerender->displayLists[imageIndex]);

        imageMask >>= 1;
        ++imageIndex;
    }

    free(prerender->displayLists);
}


void prerenderShiftSingleSymbol(Gfx* gfx, int xOffset, int yOffset) {
    int x = _SHIFTR(gfx->words.w0, 12, 12) + xOffset;
    int y = _SHIFTL(gfx->words.w0, 0, 12) + yOffset;

    gfx->words.w0 = _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(x, 12, 12) | _SHIFTL(y, 0, 12);

    x = _SHIFTR(gfx->words.w1, 12, 12) + xOffset;
    y = _SHIFTL(gfx->words.w1, 0, 12) + yOffset;

    gfx->words.w1 = _SHIFTL(G_TX_RENDERTILE, 24, 3) | _SHIFTL(x, 12, 12) | _SHIFTL(y, 0, 12);
}

void prerenderedTextRelocate(struct PrerenderedText* prerender, int x, int y) {
    int imageIndex = 0;
    int imageMask = prerender->usedImageIndices;

    int xOffset = (x - prerender->x) << 2;
    int yOffset = (y - prerender->y) << 2;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* gfx = prerender->displayLists[imageIndex];
            // skip color
            gfx += 2;

            while (_SHIFTR(gfx->words.w0, 24, 8) != G_ENDDL) {
                prerenderShiftSingleSymbol(gfx, xOffset, yOffset);
                gfx += 3;
            }

            osWritebackDCache(prerender->displayLists[imageIndex], (int)gfx - (int)prerender->displayLists[imageIndex]);
        }

        imageMask >>= 1;
        ++imageIndex;
    }
    prerender->x = x;
    prerender->y = y;
}

void prerenderedTextRecolor(struct PrerenderedText* prerender, struct Coloru8* color) {
    int imageIndex = 0;
    int imageMask = prerender->usedImageIndices;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* gfx = prerender->displayLists[imageIndex];

            if (color) {
                gDPPipeSync(gfx++);
                gDPSetEnvColor(gfx++, color->r, color->g, color->b, color->a);
            } else {
                gDPNoOp(gfx++);
                gDPNoOp(gfx++);
            }

            osWritebackDCache(prerender->displayLists[imageIndex], sizeof(Gfx) * 2);
        }

        imageMask >>= 1;
        ++imageIndex;
    }
}

void fontRendererFillPrerender(struct FontRenderer* renderer, struct PrerenderedText* prerender, int x, int y, struct Coloru8* color) {
    int imageIndex = 0;
    int imageMask = renderer->usedImageIndices & prerender->usedImageIndices;

    prerender->x = x;
    prerender->y = y;
    prerender->width = renderer->width;
    prerender->height = renderer->height;

    while (imageMask) {
        if (imageMask & 0x1) {
            Gfx* gfx = prerender->displayLists[imageIndex];

            if (color) {
                gDPPipeSync(gfx++);
                gDPSetEnvColor(gfx++, color->r, color->g, color->b, color->a);
            } else {
                gDPNoOp(gfx++);
                gDPNoOp(gfx++);
            }
            gfx = fontRendererBuildSingleGfx(renderer, imageIndex, x, y, gfx);
            gSPEndDisplayList(gfx++);

            osWritebackDCache(prerender->displayLists[imageIndex], (int)gfx - (int)prerender->displayLists[imageIndex]);
        }

        imageMask >>= 1;
        ++imageIndex;
    }
}

Gfx* prerenderedBatchFinish(struct PrerenderedTextBatch* batch, Gfx** fontImages, Gfx* gfx) {
    int imageIndex = 0;
    int imageMask = batch->usedImageIndices;
    int maskCheck = 1;

    while (imageMask) {
        if (imageMask & 0x1) {
            gSPDisplayList(gfx++, fontImages[imageIndex]);

            for (int i = 0; i < batch->textCount; ++i) {
                if (batch->text[i]->usedImageIndices & maskCheck) {
                    gSPDisplayList(gfx++, batch->text[i]->displayLists[imageIndex]);
                }
            }
        }

        imageMask >>= 1;
        maskCheck <<= 1;
        ++imageIndex;
    }

    stackMallocFree(batch);

    return gfx;
}