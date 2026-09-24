#ifndef __SCENE_N64_SIGNAGE_RENDER_H__
#define __SCENE_N64_SIGNAGE_RENDER_H__

// The N64 half of the chamber sign (src/scene/psp/...). signage.c decides
// what each quad shows; these write it in the machine's format.

#include "graphics/color.h"
#include "graphics/renderstate.h"
#include "scene/dynamic_render_list.h"

// Shifts a quad's UVs by a whole number of texels.
void signageVertexOffsetUV(void* vertices, int uTexels, int vTexels);

// Tints all four of a quad's corners.
void signageVertexSetColor(void* vertices, struct Coloru8* color);

// The progress bar's moving edge: model X and matching texel column.
void signageVertexSetProgress(void* vertices, short xCoord, int uTexels);

void signageRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);

#endif
