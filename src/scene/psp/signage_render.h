#ifndef __SCENE_PSP_SIGNAGE_RENDER_H__
#define __SCENE_PSP_SIGNAGE_RENDER_H__

// The PSP half of the test chamber sign; see src/scene/n64/signage_render.h.
// signage.c decides what each quad shows in texels and colours; these
// write it into the model.

#include "graphics/color.h"
#include "graphics/renderstate.h"
#include "scene/dynamic_render_list.h"

// Shifts a quad's UVs by a whole number of texels.
void signageVertexOffsetUV(void* vertices, int uTexels, int vTexels);

// Tints all four of a quad's corners.
void signageVertexSetColor(void* vertices, struct Coloru8* color);

// The progress bar's leading edge: a model X coordinate and the texel column
// that goes with it, written to the two vertices that move.
void signageVertexSetProgress(void* vertices, short xCoord, int uTexels);

void signageRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
