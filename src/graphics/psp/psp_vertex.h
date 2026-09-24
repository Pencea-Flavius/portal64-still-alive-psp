#ifndef __PSP_VERTEX_H__
#define __PSP_VERTEX_H__

#include <pspgu.h>

// The vertex layout skeletool64 emits for the PSP: the N64's numbers in the
// GU's order (weights, texture, colour, normal, position).
//
// Positions are in SCENE_SCALE units; the GU reads 16 bit as a fraction of
// 32768, which the model matrix undoes with PSP_VERTEX_POSITION_SCALE.
// UVs are normalised against the bound texture, keeping the texture matrix
// at identity (the GE ignores it when picking a mip level).

// Undoes the GU's 1/32768, leaving SCENE_SCALE units.
#define PSP_VERTEX_POSITION_UNSCALE             32768.0f

// The same in metres, for the renderer test.
#define PSP_VERTEX_POSITION_SCALE(sceneScale)   (PSP_VERTEX_POSITION_UNSCALE / (float)(sceneScale))

#define PSP_VERTEX_FORMAT_NORMAL \
    (GU_TEXTURE_32BITF | GU_NORMAL_8BIT | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D)

#define PSP_VERTEX_FORMAT_COLOR \
    (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D)

// Mirrors the N64 vertex whose cn[] holds a normal.
struct PspVertexNormal {
    float           u, v;
    signed char     nx, ny, nz;
    // The N64's Vtx_tn alpha, which the GU skips; see fadeToPrimitive.
    unsigned char   alpha;
    short           x, y, z;
    short           trailing;
};

// Mirrors the N64 vertex whose cn[] holds a vertex colour.
struct PspVertexColor {
    float           u, v;
    unsigned int    color;
    short           x, y, z;
    short           padding;
};

// Pin the layout: the GU reads raw bytes.
_Static_assert(sizeof(struct PspVertexNormal) == 20, "PspVertexNormal layout changed");
_Static_assert(__builtin_offsetof(struct PspVertexNormal, u)  == 0,  "texture must come first");
_Static_assert(__builtin_offsetof(struct PspVertexNormal, nx) == 8,  "normal must follow texture");
_Static_assert(__builtin_offsetof(struct PspVertexNormal, x)  == 12, "position must come last");

_Static_assert(sizeof(struct PspVertexColor) == 20, "PspVertexColor layout changed");
_Static_assert(__builtin_offsetof(struct PspVertexColor, u)     == 0,  "texture must come first");
_Static_assert(__builtin_offsetof(struct PspVertexColor, color) == 8,  "colour must follow texture");
_Static_assert(__builtin_offsetof(struct PspVertexColor, x)     == 12, "position must come last");

#endif
