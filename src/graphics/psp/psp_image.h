#ifndef __PSP_IMAGE_H__
#define __PSP_IMAGE_H__

// A CPU-side picture (save thumbnail, chapter screenshot) kept in a power
// of two texture and redrawn from the buffer when it changes. UVs stop at
// the used corner.

#include "psp_model.h"
#include "render_state.h"

struct PspImage {
    struct PspTexture texture;
    struct PspMaterial material;
    // PspTexture points at an array of mip levels; there is only the one.
    const void* levels[1];
    void* pixels;
    unsigned short width, height;
};

// Allocates the texture, rounded up to powers of two. Format is GU_PSM_*.
void pspImageInit(struct PspImage* image, int width, int height, unsigned int format);

// Copies a width by height buffer in, row by row.
void pspImageUpload(struct PspImage* image, const void* pixels);

void pspImageDraw(struct RenderState* renderState, struct PspImage* image, int x, int y, int width, int height, unsigned int color);

#endif
