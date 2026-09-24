#ifndef _GRAPHICS_IMAGE_H
#define _GRAPHICS_IMAGE_H

// Blits a CPU-side picture as texture tiles. N64 only; the PSP uses
// PspImage (graphics/psp/psp_image.h).

#include "graphics/color.h"
#include "graphics/renderstate.h"

void graphicsCopyImage(
    struct RenderState* state,
    void* image,
    int imageWidth,
    int imageHeight,
    int srcX,
    int srcY,
    int srcWidth,
    int srcHeight,
    int screenX,
    int screenY,
    struct Coloru8 color
);

#endif
