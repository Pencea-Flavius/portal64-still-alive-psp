#ifndef __FONT_PSP_FONT_IMAGES_H__
#define __FONT_PSP_FONT_IMAGES_H__

// The PSP half of the font atlases; see src/font/n64/font_images.h.
//
// One entry per atlas page, as on the N64, but a page is the material that
// carries the texture rather than a display list that binds it.

#include "graphics/psp/psp_model.h"

extern const struct PspMaterial* const gDejaVuSansImages[];
extern const struct PspMaterial* const gLiberationMonoImages[];

#endif
