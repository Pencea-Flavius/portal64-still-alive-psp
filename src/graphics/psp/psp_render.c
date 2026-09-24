#include "psp_render.h"

#include "psp_model.h"
#include "psp_model_render.h"

#include <pspgu.h>

// GU_SPRITES takes two vertices per quad, the top left and the bottom right.
struct PspSpriteVertex {
    unsigned int    color;
    short           x, y, z;
    short           padding;
};

// Colour 4 + vertex 6 = 10, padded to 12 to match the GE's stride.
_Static_assert(sizeof(struct PspSpriteVertex) == 12, "PspSpriteVertex layout changed");
_Static_assert(__builtin_offsetof(struct PspSpriteVertex, color) == 0, "colour first");
_Static_assert(__builtin_offsetof(struct PspSpriteVertex, x) == 4, "position last");

#define PSP_SPRITE_FORMAT \
    (GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D)

unsigned int pspRenderColor(const struct Coloru8* color) {
    return ((unsigned int)color->a << 24) |
           ((unsigned int)color->b << 16) |
           ((unsigned int)color->g << 8) |
           (unsigned int)color->r;
}

void pspRenderFillRect(
    struct RenderState* renderState,
    int x, int y, int width, int height,
    unsigned int color
) {
    struct PspSpriteVertex* vertices =
        renderStateRequestMemory(renderState, sizeof(struct PspSpriteVertex) * 2);

    if (!vertices) {
        return;
    }

    vertices[0].color = color;
    vertices[0].x = (short)x;
    vertices[0].y = (short)y;
    vertices[0].z = 0;
    vertices[0].padding = 0;

    vertices[1].color = color;
    vertices[1].x = (short)(x + width);
    vertices[1].y = (short)(y + height);
    vertices[1].z = 0;
    vertices[1].padding = 0;

    // Own state, no depth test, blending: like the N64's 2D rectangles (the
    // scene's depth test hid the level fade and the hurt overlay).
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDepthMask(1);
    sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDrawArray(GU_SPRITES, PSP_SPRITE_FORMAT, 2, 0, vertices);
    pspMaterialForgetState();
}

// Sprite UVs are texels: GU_TRANSFORM_2D does not normalise them.
//
// The GE derives the stride from the format flags, so the struct must match:
// texture 4 + colour 4 + vertex 6 = 14, padded to 16.
struct PspTexturedSpriteVertex {
    unsigned short  u, v;
    unsigned int    color;
    short           x, y, z;
};

_Static_assert(sizeof(struct PspTexturedSpriteVertex) == 16, "sprite layout changed");
_Static_assert(__builtin_offsetof(struct PspTexturedSpriteVertex, u) == 0, "texture first");
_Static_assert(__builtin_offsetof(struct PspTexturedSpriteVertex, color) == 4, "colour second");
_Static_assert(__builtin_offsetof(struct PspTexturedSpriteVertex, x) == 8, "position last");

#define PSP_TEXTURED_SPRITE_FORMAT \
    (GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D)

int pspRenderSpriteBatchBegin(struct PspSpriteBatch* batch, struct RenderState* renderState, const struct PspMaterial* material, int capacity) {
    batch->material = material;
    batch->count = 0;
    batch->capacity = 0;
    batch->vertices = NULL;

    if (!material || !material->texture || capacity <= 0) {
        return 0;
    }

    batch->vertices = renderStateRequestMemory(renderState, sizeof(struct PspTexturedSpriteVertex) * 2 * capacity);

    if (!batch->vertices) {
        return 0;
    }

    batch->capacity = capacity;
    return 1;
}

void pspRenderSpriteBatchAdd(
    struct PspSpriteBatch* batch,
    int x, int y, int width, int height,
    int u0, int v0, int u1, int v1,
    unsigned int color
) {
    if (batch->count >= batch->capacity) {
        return;
    }

    struct PspTexturedSpriteVertex* vertices = (struct PspTexturedSpriteVertex*)batch->vertices + batch->count * 2;

    vertices[0].u = (unsigned short)u0;
    vertices[0].v = (unsigned short)v0;
    vertices[0].color = color;
    vertices[0].x = (short)x;
    vertices[0].y = (short)y;
    vertices[0].z = 0;

    vertices[1].u = (unsigned short)u1;
    vertices[1].v = (unsigned short)v1;
    vertices[1].color = color;
    vertices[1].x = (short)(x + width);
    vertices[1].y = (short)(y + height);
    vertices[1].z = 0;

    ++batch->count;
}

void pspRenderSpriteBatchEnd(struct PspSpriteBatch* batch) {
    if (!batch->count) {
        return;
    }

    // Sprites carry their colour per vertex (text tint).
    pspMaterialBindSprite(batch->material);

    sceGuDrawArray(GU_SPRITES, PSP_TEXTURED_SPRITE_FORMAT, batch->count * 2, 0, batch->vertices);
    batch->count = 0;
}

void pspRenderTextureRect(
    struct RenderState* renderState,
    const struct PspMaterial* material,
    int x, int y, int width, int height,
    int u0, int v0, int u1, int v1,
    unsigned int color
) {
    struct PspSpriteBatch batch;

    if (pspRenderSpriteBatchBegin(&batch, renderState, material, 1)) {
        pspRenderSpriteBatchAdd(&batch, x, y, width, height, u0, v0, u1, v1, color);
        pspRenderSpriteBatchEnd(&batch);
    }
}

void pspRenderSetScissor(int x, int y, int width, int height) {
    // Width and height, not the far corner.
    sceGuScissor(x, y, width, height);
}
