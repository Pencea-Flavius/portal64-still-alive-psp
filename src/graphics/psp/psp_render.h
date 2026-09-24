#ifndef __PSP_RENDER_H__
#define __PSP_RENDER_H__

// The 2D primitives the menus and the HUD draw with. The render state they
// allocate out of lives in render_state.h.

#include "graphics/color.h"
#include "psp_model.h"
#include "render_state.h"

// Replaces gDPFillRectangle. Coordinates are screen pixels, colour is ABGR.
void pspRenderFillRect(
    struct RenderState* renderState,
    int x, int y, int width, int height,
    unsigned int color
);

// Replaces gSPTextureRectangle. Screen coordinates in pixels, UVs in
// texels. Takes a material, since the texture alone doesn't say how it
// combines or blends.
void pspRenderTextureRect(
    struct RenderState* renderState,
    const struct PspMaterial* material,
    int x, int y, int width, int height,
    int u0, int v0, int u1, int v1,
    unsigned int color
);

// Many textured rectangles of one material in one draw. Begin takes the
// maximum; End draws those added.
struct PspSpriteBatch {
    const struct PspMaterial* material;
    void* vertices;
    int capacity;
    int count;
};

int pspRenderSpriteBatchBegin(struct PspSpriteBatch* batch, struct RenderState* renderState, const struct PspMaterial* material, int capacity);
void pspRenderSpriteBatchAdd(
    struct PspSpriteBatch* batch,
    int x, int y, int width, int height,
    int u0, int v0, int u1, int v1,
    unsigned int color
);
void pspRenderSpriteBatchEnd(struct PspSpriteBatch* batch);

// Converts a game colour for a sprite's vertices.
unsigned int pspRenderColor(const struct Coloru8* color);

// Replaces gDPSetScissor.
void pspRenderSetScissor(int x, int y, int width, int height);

#endif
