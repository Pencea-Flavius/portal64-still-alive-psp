#ifndef __PSP_MODEL_H__
#define __PSP_MODEL_H__

#include "psp_vertex.h"

// What skeletool64 emits for a model. Unlike the N64's baked display list,
// a part names its material and the renderer applies it.

#define PSP_TEXTURE_MAX_LEVELS 8

#define PSP_COLOR_SOURCE_SHADE          0
#define PSP_COLOR_SOURCE_PRIMITIVE      1
#define PSP_COLOR_SOURCE_ENVIRONMENT    2
// The combiner's constant 1, which nothing sets at run time.
#define PSP_COLOR_SOURCE_WHITE          3

// No parent / unused entry (NO_BONE_PARENT in skeletool_armature.h).
#define PSP_NO_BONE_PARENT 0xFFFF

struct PspTexture {
    // Level 0 first, each half the previous.
    const void* const*  levels;
    unsigned char       levelCount;
    unsigned short      width;
    unsigned short      height;
    // GU_PSM_*: T4/T8 with clut for up to 256 colours, else 5551 or 8888.
    unsigned int        format;
    // Swizzled (writePspLevel()); runtime textures leave this 0.
    unsigned char       swizzled;
    // 8888 palette: 16 entries for T4, 256 for T8.
    const unsigned int* clut;
};

// A material's RDP state, reduced to what the GU can express.
//
// Blend value for additive blending scaled by alpha; set only at run time
// (the fizzle's static).
#define PSP_BLEND_ADD 2

// Both faces culled: collision/portal only, the renderer skips it.
#define PSP_CULL_BOTH 2

#define PSP_FADE_BY_ALPHA   1
#define PSP_FADE_BY_SHADE   2

struct PspMaterial {
    const char*              name;
    // Null when the material draws untextured.
    const struct PspTexture* texture;
    unsigned int             textureFunction;   // GU_TFX_*
    unsigned int             textureComponent;  // GU_TCC_RGB or GU_TCC_RGBA
    // ABGR, what GU_TFX_MODULATE multiplies the texture by.
    unsigned int             primitiveColor;
    unsigned char            textureFilter;     // GU_NEAREST or GU_LINEAR
    // GU_REPEAT or GU_CLAMP per axis, from the RDP tile. Mirrored tiles repeat
    // (the mirror is baked in).
    unsigned char            wrapS;
    unsigned char            wrapT;
    unsigned char            depthTest;
    unsigned char            depthWrite;
    // 0 opaque, 1 alpha blended, PSP_BLEND_ADD added to what is there.
    unsigned char            blend;
    unsigned char            lighting;
    // Whether back faces are dropped.
    unsigned char            cullBack;       // 1, or PSP_CULL_BOTH
    // Decals: pulled towards the camera instead of the RDP's decal depth mode.
    // 1 normal, 3 smaller minimum pull (pspDecalLight).
    unsigned char            decal;
    // PSP_COLOR_SOURCE_*: which runtime colour (primitive, environment) the
    // fragment colour and GU_TFX_BLEND's second colour stand for.
    unsigned char            fragmentSource;
    unsigned char            envSource;
    // GU_TFX_BLEND's second colour, ABGR.
    unsigned int             envColor;
    // COMBINED + COMBINED: GU_FRAGMENT_2X.
    unsigned char            colorDouble;
    // G_TEXTURE_GEN reflection mapping: GU_ENVIRONMENT_MAP against two lights
    // (see pspSetLookAt()).
    unsigned char            textureGen;
    // COMBINED * SHADE + COMBINED: doubled, with shade raised halfway to white.
    unsigned char            shadePlusOne;
    // Alpha test threshold, 0 for none.
    unsigned char            alphaTest;
    // Fog colour ABGR (G_FOG with G_RM_FOG_SHADE_A), 0 for none.
    unsigned int             fogColor;
    // (TEXEL0 - PRIMITIVE) * SHADE_ALPHA + PRIMITIVE: drawn plain, then the
    // primitive colour laid over at 1 - alpha. PSP_FADE_BY_SHADE: the same with
    // SHADE, drawn as MODULATE plus primitive * (1 - shade).
    unsigned char            fadeToPrimitive;
};

struct PspModelPart {
    // Points at an array of PspVertexNormal or PspVertexColor, per vertexFormat.
    const void*             vertices;
    const unsigned short*   indices;
    unsigned short          indexCount;
    unsigned short          vertexCount;
    // GU vertex flags, one of the PSP_VERTEX_FORMAT_* values.
    unsigned int            vertexFormat;
    const struct PspMaterial* material;
    // The bone whose space the vertices are in, -1 for model space.
    short                   boneIndex;
    // For a part across a joint: each vertex's bone (255 for none).
    // secondBoneIndex only sorted the chunk. -1 and NULL for one bone.
    short                   secondBoneIndex;
    const unsigned char*    vertexBones;
};

struct PspModel {
    const struct PspModelPart*  parts;
    unsigned short              partCount;
    // Each bone's parent, or PSP_NO_BONE_PARENT; NULL without bones.
    const unsigned short*       boneParent;
    unsigned short              boneCount;
};

#endif
