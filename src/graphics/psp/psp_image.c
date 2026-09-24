#include "psp_image.h"

#include "psp_render.h"

#include <pspkernel.h>
#include <pspgu.h>
#include <stdint.h>

#include "util/memory.h"

static unsigned short pspImageRoundUp(int value) {
    unsigned short result = 1;

    while (result < value) {
        result <<= 1;
    }

    return result;
}

void pspImageInit(struct PspImage* image, int width, int height, unsigned int format) {
    image->width = (unsigned short)width;
    image->height = (unsigned short)height;

    image->texture.width = pspImageRoundUp(width);
    image->texture.height = pspImageRoundUp(height);
    image->texture.format = format;
    image->texture.levelCount = 1;
    // Filled row by row by the CPU.
    image->texture.swizzled = 0;
    image->texture.clut = NULL;
    image->texture.levels = image->levels;

    // From the game's heap, which level loads reset along with the image's
    // owner (the C heap leaked them). Extra room to round up to 16 bytes.
    unsigned size = image->texture.width * image->texture.height * sizeof(uint16_t);
    unsigned char* block = malloc(size + 15);
    image->pixels = block ? (void*)(((unsigned)block + 15) & ~15u) : NULL;
    image->levels[0] = image->pixels;

    image->material.name = "image";
    image->material.texture = &image->texture;
    image->material.textureFunction = GU_TFX_MODULATE;
    // Colour only; the framebuffer's alpha bit is not coverage.
    image->material.textureComponent = GU_TCC_RGB;
    image->material.primitiveColor = 0xFFFFFFFF;
    image->material.textureFilter = GU_LINEAR;
    // Padded to a power of two; UVs stop at the used corner.
    image->material.wrapS = GU_CLAMP;
    image->material.wrapT = GU_CLAMP;
    image->material.depthTest = 0;
    image->material.depthWrite = 0;
    image->material.blend = 0;
    image->material.lighting = 0;
}

void pspImageUpload(struct PspImage* image, const void* pixels) {
    const uint16_t* src = pixels;
    uint16_t* dst = image->pixels;

    if (!src || !dst) {
        return;
    }

    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            dst[x + y * image->texture.width] = src[x + y * image->width];
        }
    }

    // Written back for the GE, or a new thumbnail draws black.
    sceKernelDcacheWritebackRange(dst, image->texture.width * image->texture.height * sizeof(uint16_t));
}

void pspImageDraw(struct RenderState* renderState, struct PspImage* image, int x, int y, int width, int height, unsigned int color) {
    // Same address, new pixels: flush the GE's texture cache.
    sceGuTexFlush();

    pspRenderTextureRect(
        renderState, &image->material,
        x, y, width, height,
        0, 0,
        image->width, image->height,
        color
    );
}
