#include "psp_model_render.h"

#include "system/psp/psp_profile.h"

unsigned long long gPspProfileBins[PspProfileBinCount];
unsigned gPspProfileCounters[PspProfileCounterCount];
int gPspProfileDetail;

#include "system/display.h"
#include "system/psp/display_psp.h"
#include "math/vector3.h"
#include "graphics/renderstate.h"

#include <pspge.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Extra pull towards the camera until reset (the N64's PortalFlagsZOffset),
// added to a decal's own offset.
static int   sDepthBias = 0;


// sceGuDepthRange also sets a MINZ/MAXZ window that discards pixels outside
// it. The window spans the stage's range plus the current pull (sDepthPull);
// what falls outside is how the GE clips at the near and far planes (see
// gpuClipsNear()), instead of cutting those triangles on the CPU.
static int sDepthWindowMin = 0;
// The stage's near end and the current depth offset; the window reaches
// above the near end by exactly that.
static int sDepthWindowNear = 65535;
static int sDepthPull = 0;
// Limits on z/w for a vertex the GE can take: past these the depth leaves
// 0..65535.
static float sGpuNearClipMin = -1.0f;
static float sGpuFarClipMax = 1.0f;

void pspWidenDepthWindow() {
    // One more for the rounding of the depth itself.
    int top = sDepthWindowNear + (sDepthPull > 0 ? sDepthPull : 0) + 1;

    sceGuSendCommandi(214, sDepthWindowMin);
    sceGuSendCommandi(215, top > 65535 ? 65535 : top);
}

void pspDepthResetWindow() {
    sDepthWindowMin = 0;
    sDepthWindowNear = 65535;
    sGpuNearClipMin = -1.0f;
    sGpuFarClipMax = 1.0f;
}

void pspDepthSetWindow(int nearZ, int farZ) {
    sDepthWindowNear = nearZ;
    // Room below for moving level geometry pushed back behind what is fixed.
    int bottom = farZ - PSP_DEPTH_HEADROOM;
    sDepthWindowMin = bottom < 0 ? 0 : bottom;

    // z/w -1 maps to nearZ, +1 to farZ; 0.9 leaves room for depth offsets.
    float half = (nearZ - farZ) * 0.5f;
    sGpuNearClipMin = half > 0.0f ? -1.0f - 0.9f * (65535 - nearZ) / half : -1.0f;
    // The deepest stage's far end is 0, so there it never applies.
    sGpuFarClipMax = half > 0.0f ? 1.0f + 0.9f * farZ / half : 1.0f;
}

// Skip binding the material that is already bound; only binds and draws
// touch GE state while a scene is submitted.
static int sBatching;
static const struct PspMaterial* sBatchMaterial;

void pspModelSetDepthBias(int bias) {
    // The bias is part of a bind, so force the next one.
    sBatchMaterial = NULL;
    sDepthBias = bias;
    sDepthPull = sDepthBias;
    sceGuDepthOffset(sDepthBias);
    pspWidenDepthWindow();
}

static float sDepthSlice;
static float sDepthNear;
static float sDepthFar;
static struct Vector3 sDepthEye;
static struct Vector3 sDepthForward;

void pspDepthSetView(float slice, float nearPlane, float farPlane, const struct Vector3* eye, const struct Vector3* forward) {
    sDepthSlice = slice;
    sDepthNear = nearPlane;
    sDepthFar = farPlane;
    sDepthEye = *eye;
    sDepthForward = *forward;
}

static int depthUnitsAt(float distance, float metres, int minUnits, int maxUnits);

int pspDepthUnitsFor(const struct Vector3* at, float metres, int minUnits, int maxUnits) {
    float distance = ((at->x - sDepthEye.x) * sDepthForward.x +
        (at->y - sDepthEye.y) * sDepthForward.y +
        (at->z - sDepthEye.z) * sDepthForward.z) * SCENE_SCALE;

    return depthUnitsAt(distance, metres, minUnits, maxUnits);
}

// `distance` along the view, in the scene's scaled units.
static int depthUnitsAt(float distance, float metres, int minUnits, int maxUnits) {
    if (distance < sDepthNear) {
        distance = sDepthNear;
    }

    // d(depth)/d(distance) = slice * near * far / ((far - near) * distance^2).
    float unitsPerScaled = sDepthFar > sDepthNear
        ? sDepthSlice * sDepthNear * sDepthFar / ((sDepthFar - sDepthNear) * distance * distance)
        : 0.0f;
    int units = (int)(unitsPerScaled * metres * SCENE_SCALE + 0.5f);

    return units < minUnits ? minUnits : units > maxUnits ? maxUnits : units;
}

// List space a part needs: state, matrix and clipper buffers, plus room for
// the HUD and menus.
#define PSP_LIST_RESERVE    (64 * 1024)

void pspSetLookAt(const struct Vector3* s, const struct Vector3* t) {
    ScePspFVector3 sDirection = {s->x, s->y, s->z};
    ScePspFVector3 tDirection = {t->x, t->y, t->z};

    sceGuLight(PSP_LOOKAT_LIGHT_S, GU_DIRECTIONAL, GU_DIFFUSE, &sDirection);
    sceGuLight(PSP_LOOKAT_LIGHT_T, GU_DIRECTIONAL, GU_DIFFUSE, &tDirection);
}

static int   sAutoLod = 1;
static int   sMipmaps = 1;
static float sLodBiasOverride = 0.0f;
static int   sHasBiasOverride = 0;

void pspModelSetLodOverride(int autoLod, int mipmaps, float lodBias) {
    sAutoLod = autoLod;
    sMipmaps = mipmaps;
    sLodBiasOverride = lodBias;
    sHasBiasOverride = 1;
}

// Last texture sent to the GE and its level count, to skip resending it
// (which also makes the GE reload it). Reset every frame.
static const struct PspTexture* sBoundTexture;
static unsigned char sBoundLevelCount;

// Counts frames, for what is kept only while a frame is being built.
static unsigned sFrame;

// Cached memory for vertices the CPU builds and reads back (skinning,
// recoloured copies); copied once into the list for the GE. Reading them back
// from the uncached list was 7x slower.
#define CPU_POOL_SIZE (1024 * 1024)

static unsigned char __attribute__((aligned(16))) sCpuPool[CPU_POOL_SIZE];
static unsigned sCpuPoolUsed;

// NULL when full; the caller builds in the list instead.
static void* cpuPoolAlloc(unsigned size) {
    unsigned aligned = (sCpuPoolUsed + 15) & ~15u;

    if (aligned + size > CPU_POOL_SIZE) {
        return NULL;
    }

    sCpuPoolUsed = aligned + size;
    return &sCpuPool[aligned];
}

// The GE's copy of vertices the CPU built, written once and never read back.
static void* geCopy(const void* vertices, unsigned size) {
    void* result = sceGuGetMemory(size);
    memcpy(result, vertices, size);
    return result;
}

// The projection times the view, kept until either changes.
static ScePspFMatrix4 sProjectionView;
static int sProjectionViewValid;

void pspModelViewChanged() {
    sProjectionViewValid = 0;
}

void pspModelStartFrame() {
    sProjectionViewValid = 0;
    pspMaterialForgetState();
    sBoundTexture = NULL;
    sCpuPoolUsed = 0;
    ++sFrame;
}

static void materialBindState(const struct PspMaterial* material, int isSprite);

static float sFogNear = 1.0f;
static float sFogFar = 2.0f;

void pspSetFogRange(float nearDistance, float farDistance) {
    sFogNear = nearDistance;
    sFogFar = farDistance;
}

static int textureLevelSize(int size, int level) {
    size >>= level;
    return size < 1 ? 1 : size;
}

// Narrow levels are stored padded, so the stride stops shrinking before the
// width does: 8 texels, and 16 bytes a row at least. This has to agree with
// the generator's pspMinBufferWidth().
static int textureBufferWidth(const struct PspTexture* texture, int levelWidth) {
    int minWidth = texture->format == GU_PSM_T4 ? 32 : texture->format == GU_PSM_T8 ? 16 : 8;
    return levelWidth < minWidth ? minWidth : levelWidth;
}

static int textureBitsPerTexel(const struct PspTexture* texture) {
    switch (texture->format) {
        case GU_PSM_T4:
            return 4;
        case GU_PSM_T8:
            return 8;
        case GU_PSM_8888:
            return 32;
        default:
            return 16;
    }
}

// Copy textures into free VRAM (~1.2MB), where the GE samples them faster.
// Whole textures only; what does not fit stays in main memory.
static unsigned char* sVramNext;
static unsigned char* sVramEnd;

static unsigned textureCopySize(unsigned bytes) {
    return (bytes + 15) & ~15u;
}

static void textureToVram(struct PspTexture* texture) {
    // Already copied, through another material.
    const unsigned char* vram = (const unsigned char*)sceGeEdramGetAddr();

    if ((const unsigned char*)texture->levels[0] >= vram && (const unsigned char*)texture->levels[0] < sVramEnd) {
        return;
    }

    const int bits = textureBitsPerTexel(texture);
    const unsigned clutBytes = texture->clut ? (texture->format == GU_PSM_T4 ? 16 : 256) * sizeof(unsigned int) : 0;
    unsigned levelBytes[PSP_TEXTURE_MAX_LEVELS];
    unsigned total = textureCopySize(clutBytes);

    for (int level = 0; level < texture->levelCount; ++level) {
        int levelWidth = textureLevelSize(texture->width, level);
        levelBytes[level] = textureBufferWidth(texture, levelWidth) * textureLevelSize(texture->height, level) * bits / 8;
        total += textureCopySize(levelBytes[level]);
    }

    if (total > (unsigned)(sVramEnd - sVramNext)) {
        return;
    }

    const void** levels = (const void**)texture->levels;

    for (int level = 0; level < texture->levelCount; ++level) {
        memcpy(sVramNext, levels[level], levelBytes[level]);
        levels[level] = sVramNext;
        sVramNext += textureCopySize(levelBytes[level]);
    }

    if (clutBytes) {
        memcpy(sVramNext, texture->clut, clutBytes);
        texture->clut = (const unsigned int*)sVramNext;
        sVramNext += textureCopySize(clutBytes);
    }
}

void pspMaterialsToVram(const struct PspMaterial* const* materials, int count) {
    if (!sVramNext) {
        unsigned size;
        sVramNext = displayPspFreeVram(&size);
        sVramEnd = sVramNext + size;
    }

    for (int i = 0; i < count; ++i) {
        if (materials[i] && materials[i]->texture) {
            textureToVram((struct PspTexture*)materials[i]->texture);
        }
    }

    sceKernelDcacheWritebackAll();
}

static void materialBind(const struct PspMaterial* material, int isSprite) {
    unsigned long long bindStart = pspProfileDetailNow();
    pspProfileCount(PspProfileMaterialBinds, 1);
    materialBindState(material, isSprite);
    pspProfileDetailAdd(PspProfileBind, bindStart);
}

// What materialBindState() last sent, so a bind sends only changes. Code that
// sets the same state directly calls pspMaterialForgetState() after.
enum GuValue {
    GuTexFunction,
    GuTexFilter,
    GuTexScale,
    GuTexWrap,
    GuTexMapMode,
    GuColor,
    GuDepthMask,
    GuBlendFunc,
    GuValueCount,
};

static unsigned sGuValue[GuValueCount];
static unsigned sGuValuesKnown;
// One bit per GU_* state, all of which are under 32.
static unsigned sGuStatesKnown;
static unsigned sGuStatesOn;

void pspMaterialForgetState() {
    sGuValuesKnown = 0;
    sGuStatesKnown = 0;
}

// Nonzero if `which` changed; records `value` as sent.
static int guValueChanged(enum GuValue which, unsigned value) {
    unsigned bit = 1u << which;

    if ((sGuValuesKnown & bit) && sGuValue[which] == value) {
        return 0;
    }

    sGuValuesKnown |= bit;
    sGuValue[which] = value;
    return 1;
}

static void guState(int state, int on) {
    unsigned bit = 1u << state;

    if ((sGuStatesKnown & bit) && ((sGuStatesOn & bit) != 0) == (on != 0)) {
        return;
    }

    sGuStatesKnown |= bit;

    if (on) {
        sGuStatesOn |= bit;
        sceGuEnable(state);
    } else {
        sGuStatesOn &= ~bit;
        sceGuDisable(state);
    }
}

static void materialBindState(const struct PspMaterial* material, int isSprite) {
    const int mipmapped = !isSprite;

    if (material->texture) {
        const struct PspTexture* texture = material->texture;

        guState(GU_TEXTURE_2D, 1);
        unsigned char levelCount = (mipmapped && sMipmaps) ? texture->levelCount : 1;

        if (texture != sBoundTexture || levelCount != sBoundLevelCount) {
            sBoundTexture = texture;
            sBoundLevelCount = levelCount;
            pspProfileCount(PspProfileTextureBinds, 1);

            // maxmips is the highest level index, not how many there are.
            sceGuTexMode(texture->format, levelCount - 1, 0, texture->swizzled ? GU_TRUE : GU_FALSE);

            // The palette goes to the GE's own memory, in blocks of eight.
            if (texture->clut) {
                sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
                sceGuClutLoad(texture->format == GU_PSM_T4 ? 2 : 32, texture->clut);
            }

            for (unsigned char level = 0; level < levelCount; ++level) {
                int levelWidth = textureLevelSize(texture->width, level);

                sceGuTexImage(level, levelWidth, textureLevelSize(texture->height, level),
                    textureBufferWidth(texture, levelWidth), texture->levels[level]);
            }
        }

        if (guValueChanged(GuTexFunction, material->textureFunction | (material->textureComponent << 8))) {
            sceGuTexFunc(material->textureFunction, material->textureComponent);
        }

        if (material->textureFunction == GU_TFX_BLEND) {
            sceGuTexEnvColor(material->envColor);
        }

        if (levelCount > 1) {
            float bias = sHasBiasOverride ? sLodBiasOverride : 0.0f;

            sceGuTexLevelMode(sAutoLod ? GU_TEXTURE_AUTO : GU_TEXTURE_CONST, bias);
            if (guValueChanged(GuTexFilter, GU_LINEAR_MIPMAP_LINEAR | (material->textureFilter << 8))) {
                sceGuTexFilter(GU_LINEAR_MIPMAP_LINEAR, material->textureFilter);
            }
        } else {
            if (guValueChanged(GuTexFilter, material->textureFilter | (material->textureFilter << 8))) {
                sceGuTexFilter(material->textureFilter, material->textureFilter);
            }
        }

        // Model UVs are already normalised, so no texture scale (it would also skew
        // mip selection). Sprites use texels and never get here.
        if (guValueChanged(GuTexScale, 1)) {
            sceGuTexScale(1.0f, 1.0f);
            sceGuTexOffset(0.0f, 0.0f);
        }
        // Set the wrap explicitly. Mirroring is baked into the texture, so mirrored
        // tiles repeat.
        if (guValueChanged(GuTexWrap, material->wrapS | (material->wrapT << 8))) {
            sceGuTexWrap(material->wrapS, material->wrapT);
        }
    } else {
        guState(GU_TEXTURE_2D, 0);
    }

    // Reflection mapping: UVs from the normal against lights 2 and 3, which
    // pspSetLookAt() aims like the N64's look at. They are never enabled.
    if (material->textureGen && !isSprite && material->texture) {
        if (guValueChanged(GuTexMapMode, GU_ENVIRONMENT_MAP)) {
            sceGuTexMapMode(GU_ENVIRONMENT_MAP, PSP_LOOKAT_LIGHT_S, PSP_LOOKAT_LIGHT_T);
        }
    } else if (guValueChanged(GuTexMapMode, GU_TEXTURE_COORDS)) {
        sceGuTexMapMode(GU_TEXTURE_COORDS, 0, 0);
    }

    // Set explicitly, or the last draw decides.
    if (material->colorDouble && !isSprite) {
        guState(GU_FRAGMENT_2X, 1);
    } else {
        guState(GU_FRAGMENT_2X, 0);
    }

    // The RDP's primitive colour; vertex colours multiply on top.
    if (guValueChanged(GuColor, material->primitiveColor)) {
        sceGuColor(material->primitiveColor);
    }

    // Sprites ignore depth, like the N64's 2D rectangles.
    if (material->depthTest && !isSprite) {
        guState(GU_DEPTH_TEST, 1);
    } else {
        guState(GU_DEPTH_TEST, 0);
    }

    // The argument disables writing, so it reads inverted.
    const int depthMask = (material->depthWrite && !isSprite) ? 0 : 1;

    if (guValueChanged(GuDepthMask, depthMask)) {
        sceGuDepthMask(depthMask);
    }

    // A sprite is flat on the screen and never culled.
    if (material->cullBack && !isSprite) {
        guState(GU_CULL_FACE, 1);
    } else {
        guState(GU_CULL_FACE, 0);
    }

    // Depth is reversed, so pulling towards the camera is positive.
    sDepthPull = ((material->decal && !isSprite) ? PSP_DECAL_DEPTH_OFFSET : 0) + sDepthBias;
    sceGuDepthOffset(sDepthPull);
    pspWidenDepthWindow();

    // Fog like the N64's G_RM_FOG_SHADE_A. The GE wants the distances negated;
    // positive ones never showed any fog.
    if (material->fogColor && !isSprite) {
        guState(GU_FOG, 1);
        sceGuFog(-sFogNear, -sFogFar, material->fogColor & 0x00FFFFFF);
    } else {
        guState(GU_FOG, 0);
    }

    // A cut-out's holes are not drawn at all.
    if (material->alphaTest) {
        guState(GU_ALPHA_TEST, 1);
        sceGuAlphaFunc(GU_GEQUAL, material->alphaTest, 0xFF);
    } else {
        guState(GU_ALPHA_TEST, 0);
    }

    // The material decides lighting on/off; the scene sets the lights.
    if (material->lighting) {
        guState(GU_LIGHTING, 1);
    } else {
        guState(GU_LIGHTING, 0);
    }

    // Sprites always blend: fades (like the intro logo) live in the alpha.
    if (material->blend == PSP_BLEND_ADD && !isSprite) {
        guState(GU_BLEND, 1);

        if (guValueChanged(GuBlendFunc, PSP_BLEND_ADD)) {
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
        }
    } else if (material->blend || isSprite) {
        guState(GU_BLEND, 1);

        if (guValueChanged(GuBlendFunc, 1)) {
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        }
    } else {
        guState(GU_BLEND, 0);
    }
}

// The last bound material, used by parts without their own (a level chunk
// gets the scene's materialIndex).
static const struct PspMaterial* sBoundMaterial;
// A part's own material, bound only if the part is actually drawn, so culled
// parts cost no bind.
static const struct PspMaterial* sPendingMaterial;


void pspModelBeginBatch() {
    sBatching = 1;
    sBatchMaterial = NULL;
}

void pspModelEndBatch() {
    sBatching = 0;
    sBatchMaterial = NULL;
}

void pspMaterialBind(const struct PspMaterial* material) {
    sPendingMaterial = NULL;
    sBoundMaterial = material;

    if (sBatching && material == sBatchMaterial) {
        pspProfileCount(PspProfileMaterialBindsSkipped, 1);
        return;
    }

    materialBind(material, 0);

    if (sBatching) {
        sBatchMaterial = material;
    }
}

void pspMaterialBindSprite(const struct PspMaterial* material) {
    materialBind(material, 1);
}

// Max bone chain depth; guards against a broken table looping.
#define PSP_MAX_BONE_DEPTH 16

#define PSP_MAX_BONES 256

// The pose copied out of the uncached frame scratch, keyed by address and
// frame, since bone chains read it many times.
static ScePspFMatrix4 sPose[PSP_MAX_BONES];
static const ScePspFMatrix4* sPoseSource;
static int sPoseCount;
static unsigned sPoseFrame;

static const ScePspFMatrix4* poseCached(const ScePspFMatrix4* bones, int count) {
    if (count > PSP_MAX_BONES) {
        return bones;
    }

    if (bones != sPoseSource || sPoseFrame != sFrame || count > sPoseCount) {
        memcpy(sPose, bones, sizeof(ScePspFMatrix4) * count);
        sPoseSource = bones;
        sPoseCount = count;
        sPoseFrame = sFrame;
    }

    return sPose;
}

// A bone's matrix in model space: the pose is parent-relative, so multiply
// down the chain from the root (the N64 pushes it on the matrix stack).
static int boneWorldMatrix(
    const struct PspModel* model,
    const ScePspFMatrix4* bones,
    int boneIndex,
    ScePspFMatrix4* out
) {
    unsigned short chain[PSP_MAX_BONE_DEPTH];
    int depth = 0;
    int bone = boneIndex;

    while (bone >= 0 && bone < model->boneCount && depth < PSP_MAX_BONE_DEPTH) {
        chain[depth++] = (unsigned short)bone;

        unsigned short parent = model->boneParent[bone];
        bone = parent == PSP_NO_BONE_PARENT ? -1 : (int)parent;
    }

    if (!depth) {
        return 0;
    }

    *out = bones[chain[depth - 1]];

    for (int i = depth - 2; i >= 0; --i) {
        ScePspFMatrix4 product;
        gumMultMatrix(&product, out, &bones[chain[i]]);
        *out = product;
    }

    return 1;
}

// The GE drops whole triangles that have a vertex behind the camera or
// outside its 4096 pixel space (PPSSPP draws them anyway). Like other PSP
// ports of N64 games, clip those on the CPU.

// How far past the viewport edge, in half viewports, a vertex may land before
// the CPU cuts it. The GE space allows ~8.5 widths and 15 heights, but at
// that much cut corners landed on its edge and walls vanished up close.
#define PSP_GUARD_BAND_X 4.0f
#define PSP_GUARD_BAND_Y 7.0f

// Cut just behind the near plane so corners never round past it.
#define PSP_NEAR_CUT (1.0f - 0.001f)

// Clipper buffer sizes; bigger parts are drawn unclipped.
#define PSP_CLIP_MAX_VERTICES   4096
#define PSP_CLIP_MAX_OUTPUT     4096
#define PSP_CLIP_MAX_INDICES    16384

#define CLIP_PLANE_COUNT 6

// All floats, so interpolating is one loop. Colour and normal share the tail
// (a format has one or the other).
struct ClipVertex {
    float clip[4];
    float pos[3];
    float u, v;
    union {
        float color[4];
        float normal[3];
    };
};

#define CLIP_VERTEX_FLOATS_COLOR    13
#define CLIP_VERTEX_FLOATS_NORMAL   12

// A clipped vertex; positions are floats already unscaled from 16 bit.
struct PspClippedNormal {
    float u, v;
    float nx, ny, nz;
    float x, y, z;
};

struct PspClippedColor {
    float u, v;
    unsigned int color;
    float x, y, z;
};

#define PSP_CLIPPED_FORMAT_NORMAL \
    (GU_TEXTURE_32BITF | GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D)
#define PSP_CLIPPED_FORMAT_COLOR \
    (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)

// Inside when positive. Near is z = -w, far z = w (the N64's convention).
static float clipPlaneDistance(const float* c, int plane) {
    switch (plane) {
        case 0: return c[2] + PSP_NEAR_CUT * c[3];
        case 1: return c[3] - c[2];
        case 2: return c[0] + PSP_GUARD_BAND_X * c[3];
        case 3: return PSP_GUARD_BAND_X * c[3] - c[0];
        case 4: return c[1] + PSP_GUARD_BAND_Y * c[3];
        default: return PSP_GUARD_BAND_Y * c[3] - c[1];
    }
}

// In a portal view, the exit portal's plane. Triangles fully behind it are
// the back of the exit wall and are dropped; crossing ones are kept, as on
// the N64. Tested in part space through the model matrix.
#define STAGE_PLANE_BIT (1 << 6)

static int sStagePlaneActive;
static int sSkinnedDraw;
static float sStagePlaneWorld[4];

void pspSetStageCullPlane(const float* worldPlane) {
    sStagePlaneActive = worldPlane != NULL;

    if (worldPlane) {
        for (int i = 0; i < 4; ++i) {
            sStagePlaneWorld[i] = worldPlane[i];
        }
    }
}

// A bit per plane where clipPlaneDistance() < 0, unrolled since it runs for
// thousands of vertices a frame.
static inline unsigned clipOutcode(const float* c) {
    float bandX = PSP_GUARD_BAND_X * c[3];
    float bandY = PSP_GUARD_BAND_Y * c[3];

    return (c[2] + c[3] < 0.0f)
        | ((c[3] - c[2] < 0.0f) << 1)
        | ((c[0] + bandX < 0.0f) << 2)
        | ((bandX - c[0] < 0.0f) << 3)
        | ((c[1] + bandY < 0.0f) << 4)
        | ((bandY - c[1] < 0.0f) << 5);
}

static float sClipCoords[PSP_CLIP_MAX_VERTICES][4] __attribute__((aligned(16)));

// VFPU vertex transforms. gum keeps its matrix in M3, so these use M0 and M1
// and reload M0 after anything that may call gum. The matrix goes in as M0's
// columns and is read transposed (E000).
static inline void vfpuLoadMatrix(const ScePspFMatrix4* m) {
    __asm__ volatile(
        "lv.q C000,  0(%0)\n"
        "lv.q C010, 16(%0)\n"
        "lv.q C020, 32(%0)\n"
        "lv.q C030, 48(%0)\n"
        : : "r"(m) : "memory");
}

// out = M0 * (x, y, z, 1); `out` is four floats on a 16 byte boundary.
static inline void vfpuTransformPoint(int x, int y, int z, float* out) {
    __asm__ volatile(
        "mtv %1, S100\n"
        "mtv %2, S101\n"
        "mtv %3, S102\n"
        "vi2f.t C100, C100, 0\n"
        "vone.s S103\n"
        "vtfm4.q C110, E000, C100\n"
        "sv.q C110, 0(%0)\n"
        : : "r"(out), "r"(x), "r"(y), "r"(z) : "memory");
}

// out = M0's upper 3x3 * (x, y, z), a direction; four floats out as above.
static inline void vfpuTransformDirection(int x, int y, int z, float* out) {
    __asm__ volatile(
        "mtv %1, S120\n"
        "mtv %2, S121\n"
        "mtv %3, S122\n"
        "vi2f.t C120, C120, 0\n"
        "vtfm3.t C130, E000, C120\n"
        "sv.q C130, 0(%0)\n"
        : : "r"(out), "r"(x), "r"(y), "r"(z) : "memory");
}

// Whether the GE can clip this vertex at the near plane itself: in front of
// the camera and with depth still under 65535. The depth window drops the
// pixels in front of the plane.
static int gpuClipsNear(int index) {
    const float* c = sClipCoords[index];
    return c[3] > 0.0f && c[2] >= sGpuNearClipMin * c[3];
}

// Same for the far plane: depth still above 0.
static int gpuClipsFar(int index) {
    const float* c = sClipCoords[index];
    return c[3] > 0.0f && c[2] <= sGpuFarClipMax * c[3];
}

// Whether a triangle crossing only near/far can go to the GE whole.
static int gpuClipsDepth(unsigned codeOr, int i0, int i1, int i2) {
    if (codeOr & ~3u) {
        return 0;
    }

    if ((codeOr & 1) && !(gpuClipsNear(i0) && gpuClipsNear(i1) && gpuClipsNear(i2))) {
        return 0;
    }

    return !(codeOr & 2) || (gpuClipsFar(i0) && gpuClipsFar(i1) && gpuClipsFar(i2));
}
static unsigned char sClipCodes[PSP_CLIP_MAX_VERTICES];

// Both vertex layouts keep the position at the same offset.
static const short* vertexPosition(const struct PspModelPart* part, int index) {
    return (const short*)((const char*)part->vertices + index * sizeof(struct PspVertexNormal) + 12);
}

// Bounding spheres, so most parts skip the per-vertex edge test (6ms a frame
// in a busy room). Cached only for generated geometry (between _fdata and
// _edata); runtime geometry reuses addresses. Skinned parts are not covered.
#define PART_BOUNDS_SLOTS   8192
#define PART_BOUNDS_PROBES  16

struct PartBounds {
    const void* key;
    float center[3];
    float radius;
};

static struct PartBounds sPartBounds[PART_BOUNDS_SLOTS];

extern char _fdata[];
extern char _edata[];

static const struct PartBounds* partBounds(const struct PspModelPart* part) {
    const char* key = (const char*)part->indices;
    const char* vertices = (const char*)part->vertices;

    if (key < _fdata || key >= _edata || vertices < _fdata || vertices >= _edata || !part->vertexCount) {
        return NULL;
    }

    unsigned slot = (((unsigned)key >> 2) * 2654435761u) >> (32 - 13);

    for (int probe = 0; probe < PART_BOUNDS_PROBES; ++probe, slot = (slot + 1) & (PART_BOUNDS_SLOTS - 1)) {
        struct PartBounds* bounds = &sPartBounds[slot];

        if (bounds->key == key) {
            return bounds;
        }

        if (bounds->key) {
            continue;
        }

        float min[3], max[3];

        for (int axis = 0; axis < 3; ++axis) {
            min[axis] = max[axis] = vertexPosition(part, 0)[axis];
        }

        for (int i = 1; i < part->vertexCount; ++i) {
            const short* p = vertexPosition(part, i);

            for (int axis = 0; axis < 3; ++axis) {
                min[axis] = p[axis] < min[axis] ? p[axis] : min[axis];
                max[axis] = p[axis] > max[axis] ? p[axis] : max[axis];
            }
        }

        float radiusSquared = 0.0f;

        for (int axis = 0; axis < 3; ++axis) {
            bounds->center[axis] = (min[axis] + max[axis]) * 0.5f;
        }

        for (int i = 0; i < part->vertexCount; ++i) {
            const short* p = vertexPosition(part, i);
            float dx = p[0] - bounds->center[0];
            float dy = p[1] - bounds->center[1];
            float dz = p[2] - bounds->center[2];
            float distanceSquared = dx * dx + dy * dy + dz * dz;
            radiusSquared = distanceSquared > radiusSquared ? distanceSquared : radiusSquared;
        }

        bounds->radius = sqrtf(radiusSquared);
        bounds->key = key;
        return bounds;
    }

    // Table full: test per vertex.
    return NULL;
}

#define SPHERE_INSIDE   0
#define SPHERE_OUTSIDE  1
#define SPHERE_CROSSES  2

// Each clip plane is linear in part space: its value at the centre plus or
// minus radius times its gradient bounds it over the sphere.
static const float sPlaneCoefficients[CLIP_PLANE_COUNT][4] = {
    {0.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, -1.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, PSP_GUARD_BAND_X},
    {-1.0f, 0.0f, 0.0f, PSP_GUARD_BAND_X},
    {0.0f, 1.0f, 0.0f, PSP_GUARD_BAND_Y},
    {0.0f, -1.0f, 0.0f, PSP_GUARD_BAND_Y},
};

// Whether a part is under PSP_MIN_PART_PIXELS across at its nearest point.
// The bar is higher through portals, where props are drawn again.
#define PSP_MIN_PART_PIXELS         2.0f
#define PSP_MIN_PART_PIXELS_PORTAL  4.5f

static float sMinPartPixels = PSP_MIN_PART_PIXELS;

void pspSetPortalView(int throughPortal) {
    sMinPartPixels = throughPortal ? PSP_MIN_PART_PIXELS_PORTAL : PSP_MIN_PART_PIXELS;
}

static int sphereTooSmall(const ScePspFMatrix4* m, const struct PartBounds* bounds) {
    const float* column[4] = {&m->x.x, &m->y.x, &m->z.x, &m->w.x};
    float w = column[0][3] * bounds->center[0] + column[1][3] * bounds->center[1] + column[2][3] * bounds->center[2] + column[3][3];
    float wReach = bounds->radius * sqrtf(column[0][3] * column[0][3] + column[1][3] * column[1][3] + column[2][3] * column[2][3]);
    float nearest = w - wReach;

    if (nearest <= 0.0f) {
        return 0;
    }

    float xReach = bounds->radius * sqrtf(column[0][0] * column[0][0] + column[1][0] * column[1][0] + column[2][0] * column[2][0]);
    float yReach = bounds->radius * sqrtf(column[0][1] * column[0][1] + column[1][1] * column[1][1] + column[2][1] * column[2][1]);
    float across = 2.0f * (xReach * (SCREEN_WD / 2) > yReach * (SCREEN_HT / 2) ? xReach * (SCREEN_WD / 2) : yReach * (SCREEN_HT / 2));

    return across < sMinPartPixels * nearest;
}

static int sphereAgainstClip(const ScePspFMatrix4* m, const struct PartBounds* bounds) {
    const float* column[4] = {&m->x.x, &m->y.x, &m->z.x, &m->w.x};
    float c[4];

    for (int j = 0; j < 4; ++j) {
        c[j] = column[0][j] * bounds->center[0] + column[1][j] * bounds->center[1] + column[2][j] * bounds->center[2] + column[3][j];
    }

    int crosses = 0;

    for (int plane = 0; plane < CLIP_PLANE_COUNT; ++plane) {
        const float* a = sPlaneCoefficients[plane];
        float value = a[0] * c[0] + a[1] * c[1] + a[2] * c[2] + a[3] * c[3];
        float gradient[3];

        for (int axis = 0; axis < 3; ++axis) {
            gradient[axis] = a[0] * column[axis][0] + a[1] * column[axis][1] + a[2] * column[axis][2] + a[3] * column[axis][3];
        }

        // Compared squared, no square root.
        float reachSquared = bounds->radius * bounds->radius *
            (gradient[0] * gradient[0] + gradient[1] * gradient[1] + gradient[2] * gradient[2]);

        if (value < 0.0f && value * value > reachSquared) {
            return SPHERE_OUTSIDE;
        }

        if (value < 0.0f || value * value < reachSquared) {
            crosses = 1;
        }
    }

    return crosses ? SPHERE_CROSSES : SPHERE_INSIDE;
}

static void loadClipVertex(const struct PspModelPart* part, int index, int isColor, struct ClipVertex* out) {
    const short* position = vertexPosition(part, index);

    for (int i = 0; i < 4; ++i) {
        out->clip[i] = sClipCoords[index][i];
    }

    out->pos[0] = position[0];
    out->pos[1] = position[1];
    out->pos[2] = position[2];

    if (isColor) {
        const struct PspVertexColor* vertex = &((const struct PspVertexColor*)part->vertices)[index];
        out->u = vertex->u;
        out->v = vertex->v;

        for (int i = 0; i < 4; ++i) {
            out->color[i] = (float)((vertex->color >> (i * 8)) & 0xFF);
        }
    } else {
        const struct PspVertexNormal* vertex = &((const struct PspVertexNormal*)part->vertices)[index];
        out->u = vertex->u;
        out->v = vertex->v;
        out->normal[0] = vertex->nx * (1.0f / 127.0f);
        out->normal[1] = vertex->ny * (1.0f / 127.0f);
        out->normal[2] = vertex->nz * (1.0f / 127.0f);
    }
}

// Sutherland-Hodgman against one plane. Order is kept, so the winding is.
// `floats` is how much of each vertex its format uses.
static int clipPolygonToPlane(const struct ClipVertex* in, int count, struct ClipVertex* out, int plane, int floats) {
    // At most nine corners after six planes.
    float distance[12];

    for (int i = 0; i < count; ++i) {
        distance[i] = clipPlaneDistance(in[i].clip, plane);
    }

    int outCount = 0;

    for (int i = 0; i < count; ++i) {
        int next = i + 1 == count ? 0 : i + 1;
        float da = distance[i];
        float db = distance[next];
        const float* fa = (const float*)&in[i];

        if (da >= 0.0f) {
            float* result = (float*)&out[outCount++];

            for (int f = 0; f < floats; ++f) {
                result[f] = fa[f];
            }
        }

        if ((da >= 0.0f) != (db >= 0.0f)) {
            float t = da / (da - db);
            const float* fb = (const float*)&in[next];
            float* result = (float*)&out[outCount++];

            for (int f = 0; f < floats; ++f) {
                result[f] = fa[f] + (fb[f] - fa[f]) * t;
            }
        }
    }

    return outCount;
}

static void storeClipped(const struct ClipVertex* in, int isColor, void* out, int index) {
    if (isColor) {
        struct PspClippedColor* vertex = &((struct PspClippedColor*)out)[index];
        unsigned color = 0;

        for (int i = 0; i < 4; ++i) {
            float channel = in->color[i] + 0.5f;
            color |= (unsigned)(channel < 0.0f ? 0.0f : channel > 255.0f ? 255.0f : channel) << (i * 8);
        }

        vertex->u = in->u;
        vertex->v = in->v;
        vertex->color = color;
        vertex->x = in->pos[0];
        vertex->y = in->pos[1];
        vertex->z = in->pos[2];
    } else {
        struct PspClippedNormal* vertex = &((struct PspClippedNormal*)out)[index];
        vertex->u = in->u;
        vertex->v = in->v;
        vertex->nx = in->normal[0];
        vertex->ny = in->normal[1];
        vertex->nz = in->normal[2];
        vertex->x = in->pos[0];
        vertex->y = in->pos[1];
        vertex->z = in->pos[2];
    }
}

// Draws a part, clipping on the CPU what the GE would drop. The model matrix
// must be current, without the 16 bit unscale.
//
// The CPU reads `part` (cached); the GE draws `geVertices`. `boundsPart` is
// the generated part whose sphere covers these positions, or NULL.
// `decalUnits` is the decal pull for this position, or -1 for the material's.
#define PSP_DECAL_PULL_METRES   0.01f
#define PSP_DECAL_MIN_UNITS     4
// Decal 3 (pspDecalLight): something stands right in front of it.
#define PSP_DECAL_MIN_UNITS_LIGHT   2

static int sPartDecalUnits = -1;

static void submitDraw(int vertexFormat, int count, const void* indices, const void* vertices) {
    if (sPendingMaterial) {
        pspMaterialBind(sPendingMaterial);
    }

    if (sPartDecalUnits >= 0) {
        sDepthPull = sPartDecalUnits + sDepthBias;
        sceGuDepthOffset(sDepthPull);
        pspWidenDepthWindow();
    }

    unsigned long long submitStart = pspProfileDetailNow();
    sceGumDrawArray(GU_TRIANGLES, vertexFormat, count, indices, vertices);
    pspProfileCount(PspProfileDrawCalls, 1);
    pspProfileDetailAdd(PspProfileSubmit, submitStart);
}

// Set around the view model: always on screen, so skip the edge test.
static int sNoClip;

void pspModelSetNoClip(int noClip) {
    sNoClip = noClip;
}

static void drawPartClipped(const struct PspModelPart* part, const void* geVertices, const struct PspModelPart* boundsPart, const ScePspFMatrix4* projectionView, const ScePspFVector3* vertexScale) {
    const int isColor = part->vertexFormat == PSP_VERTEX_FORMAT_COLOR;

    sPartDecalUnits = -1;

    const int canClip = !sNoClip && (isColor || part->vertexFormat == PSP_VERTEX_FORMAT_NORMAL)
        && part->vertexCount <= PSP_CLIP_MAX_VERTICES;

    unsigned anyOutside = 0;
    unsigned allOutside = ~0u;

    pspProfileCount(PspProfileParts, 1);
    pspProfileCount(PspProfileTriangles, part->indexCount / 3);

    unsigned long long clipStart = pspProfileDetailNow();

    int sphere = SPHERE_CROSSES;

    if (canClip) {
        ScePspFMatrix4 model;
        ScePspFMatrix4 m;
        sceGumStoreMatrix(&model);
        gumMultMatrix(&m, projectionView, &model);

        const struct PartBounds* bounds = boundsPart ? partBounds(boundsPart) : NULL;

        if (bounds) {
            sphere = sphereTooSmall(&m, bounds) ? SPHERE_OUTSIDE : sphereAgainstClip(&m, bounds);

            const struct PspMaterial* drawn = sPendingMaterial ? sPendingMaterial : sBoundMaterial;

            if (drawn && drawn->decal) {
                float w = m.x.w * bounds->center[0] + m.y.w * bounds->center[1] + m.z.w * bounds->center[2] + m.w.w;
                sPartDecalUnits = depthUnitsAt(w, PSP_DECAL_PULL_METRES,
                    drawn->decal == 3 ? PSP_DECAL_MIN_UNITS_LIGHT : PSP_DECAL_MIN_UNITS, PSP_DECAL_DEPTH_OFFSET);
            }
        }

        // The exit portal's plane in the part's own space: a * M.
        float plane[4];

        if (sStagePlaneActive && !sSkinnedDraw) {
            const float* column[4] = {&model.x.x, &model.y.x, &model.z.x, &model.w.x};

            for (int j = 0; j < 4; ++j) {
                plane[j] = sStagePlaneWorld[0] * column[j][0] + sStagePlaneWorld[1] * column[j][1] +
                    sStagePlaneWorld[2] * column[j][2] + sStagePlaneWorld[3] * column[j][3];
            }

            if (bounds && sphere != SPHERE_OUTSIDE) {
                float value = plane[0] * bounds->center[0] + plane[1] * bounds->center[1] + plane[2] * bounds->center[2] + plane[3];
                float reachSquared = bounds->radius * bounds->radius * (plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);

                if (value < 0.0f && value * value > reachSquared) {
                    sphere = SPHERE_OUTSIDE;
                } else if (value < 0.0f || value * value < reachSquared) {
                    sphere = SPHERE_CROSSES;
                }
            }
        }

        if (sphere == SPHERE_OUTSIDE) {
            pspProfileDetailAdd(PspProfileClip, clipStart);
            pspProfileCount(PspProfilePartsCulled, 1);
            return;
        }

        if (sphere == SPHERE_CROSSES) {
            pspProfileCount(PspProfileVertices, part->vertexCount);
        }

        if (sphere == SPHERE_CROSSES) {
            vfpuLoadMatrix(&m);
        }

        for (int i = 0; sphere == SPHERE_CROSSES && i < part->vertexCount; ++i) {
            const short* p = vertexPosition(part, i);
            float* c = sClipCoords[i];

            vfpuTransformPoint(p[0], p[1], p[2], c);

            sClipCodes[i] = clipOutcode(c);

            if (sStagePlaneActive && !sSkinnedDraw && plane[0] * p[0] + plane[1] * p[1] + plane[2] * p[2] + plane[3] < 0.0f) {
                sClipCodes[i] |= STAGE_PLANE_BIT;
            }

            anyOutside |= sClipCodes[i];
            allOutside &= sClipCodes[i];
        }
    }

    pspProfileDetailAdd(PspProfileClip, clipStart);

    if (!canClip || !anyOutside) {
        sceGumScale(vertexScale);
        submitDraw(part->vertexFormat, part->indexCount, part->indices, geVertices);
        return;
    }

    if (allOutside) {
        pspProfileCount(PspProfilePartsCulled, 1);
        return;
    }

    // Includes the draws of the CPU's cut triangles, which submit counts too.
    unsigned long long cutStart = pspProfileDetailNow();

    // Triangles that need no cut are gathered in cached memory and drawn
    // together; if all are kept, the part's own index buffer is drawn.
    static unsigned short sKept[PSP_CLIP_MAX_INDICES];
    unsigned short* kept = part->indexCount <= PSP_CLIP_MAX_INDICES
        ? sKept
        : sceGuGetMemory(sizeof(unsigned short) * part->indexCount);
    int keptCount = 0;

    static struct ClipVertex polygonA[12];
    static struct ClipVertex polygonB[12];
    static unsigned char clippedBuffer[PSP_CLIP_MAX_OUTPUT * sizeof(struct PspClippedNormal)];
    int clippedCount = 0;

    // Flush when full. Cut triangles go from the cached pool.
    #define FLUSH_CLIPPED() do { \
        if (clippedCount) { \
            unsigned size = clippedCount * (isColor ? sizeof(struct PspClippedColor) : sizeof(struct PspClippedNormal)); \
            void* clipped = cpuPoolAlloc(size); \
            if (clipped) { \
                memcpy(clipped, clippedBuffer, size); \
                sceKernelDcacheWritebackRange(clipped, size); \
            } else { \
                clipped = sceGuGetMemory(size); \
                memcpy(clipped, clippedBuffer, size); \
            } \
            submitDraw(isColor ? PSP_CLIPPED_FORMAT_COLOR : PSP_CLIPPED_FORMAT_NORMAL, clippedCount, NULL, clipped); \
            clippedCount = 0; \
        } \
    } while (0)

    for (int t = 0; t + 2 < part->indexCount; t += 3) {
        unsigned short i0 = part->indices[t];
        unsigned short i1 = part->indices[t + 1];
        unsigned short i2 = part->indices[t + 2];
        // Fully behind the exit plane is dropped below; crossing is not cut.
        unsigned codeOr = (sClipCodes[i0] | sClipCodes[i1] | sClipCodes[i2]) & ~STAGE_PLANE_BIT;

        if (!codeOr && !(sClipCodes[i0] & sClipCodes[i1] & sClipCodes[i2])) {
            kept[keptCount++] = i0;
            kept[keptCount++] = i1;
            kept[keptCount++] = i2;
            continue;
        }

        if (sClipCodes[i0] & sClipCodes[i1] & sClipCodes[i2]) {
            continue;
        }

        if (gpuClipsDepth(codeOr, i0, i1, i2)) {
            kept[keptCount++] = i0;
            kept[keptCount++] = i1;
            kept[keptCount++] = i2;
            continue;
        }

        pspProfileCount(PspProfileTrianglesClipped, 1);

        if (codeOr & 1) {
            pspProfileCount(PspProfileTrianglesClippedNear, 1);
        }

        if (codeOr & 2) {
            pspProfileCount(PspProfileTrianglesClippedFar, 1);
        }

        if (codeOr & ~3u) {
            pspProfileCount(PspProfileTrianglesClippedBand, 1);
        }

        loadClipVertex(part, i0, isColor, &polygonA[0]);
        loadClipVertex(part, i1, isColor, &polygonA[1]);
        loadClipVertex(part, i2, isColor, &polygonA[2]);

        struct ClipVertex* in = polygonA;
        struct ClipVertex* out = polygonB;
        int count = 3;

        for (int plane = 0; plane < CLIP_PLANE_COUNT && count >= 3; ++plane) {
            if (!(codeOr & (1 << plane))) {
                continue;
            }

            count = clipPolygonToPlane(in, count, out, plane, isColor ? CLIP_VERTEX_FLOATS_COLOR : CLIP_VERTEX_FLOATS_NORMAL);

            struct ClipVertex* swap = in;
            in = out;
            out = swap;
        }

        // A fan keeps the winding; nine corners make at most seven triangles.
        if (clippedCount + 21 > PSP_CLIP_MAX_OUTPUT) {
            FLUSH_CLIPPED();
        }

        for (int j = 1; j + 1 < count; ++j) {
            storeClipped(&in[0], isColor, clippedBuffer, clippedCount++);
            storeClipped(&in[j], isColor, clippedBuffer, clippedCount++);
            storeClipped(&in[j + 1], isColor, clippedBuffer, clippedCount++);
        }
    }

    FLUSH_CLIPPED();
    #undef FLUSH_CLIPPED

    const unsigned short* keptIndices = kept;

    if (keptCount == part->indexCount) {
        keptIndices = part->indices;
    } else if (keptCount && kept == sKept) {
        unsigned short* copy = sceGuGetMemory(sizeof(unsigned short) * keptCount);
        memcpy(copy, sKept, sizeof(unsigned short) * keptCount);
        keptIndices = copy;
    }

    pspProfileDetailAdd(PspProfileCut, cutStart);

    if (keptCount) {
        sceGumScale(vertexScale);
        submitDraw(part->vertexFormat, keptCount, keptIndices, geVertices);
    }
}

// A part across a joint, skinned on the CPU per vertex and written back in
// the part's layout, then drawn through the clipper like any part. Positions
// round back to the 16 bit grid.
static short roundToShort(float value) {
    value += value < 0.0f ? -0.5f : 0.5f;

    if (value > 32767.0f) {
        return 32767;
    }

    if (value < -32768.0f) {
        return -32768;
    }

    return (short)value;
}

static signed char roundToNormal(float value) {
    float scaled = value * 127.0f;
    scaled += scaled < 0.0f ? -0.5f : 0.5f;
    return (signed char)(scaled > 127.0f ? 127 : scaled < -127.0f ? -127 : scaled);
}

// Skinned vertices cached by part and pose: the ball launcher and catcher
// rarely move, and reskinning cost 4ms a frame. An entry used this frame is
// never overwritten, since the GE may still read it; with none free, skin
// into the frame's memory.
#define SKIN_CACHE_SIZE 96

struct SkinCacheEntry {
    const struct PspModelPart* part;
    unsigned long long pose;
    struct PspVertexNormal* skinned;
    unsigned capacity;
    unsigned frame;
};

static struct SkinCacheEntry sSkinCache[SKIN_CACHE_SIZE];
static unsigned sSkinCacheNext;

// FNV-1a over the pose.
static unsigned long long poseHash(const ScePspFMatrix4* bones, int count) {
    const unsigned* words = (const unsigned*)bones;
    unsigned long long hash = 1469598103934665603ull;

    for (int i = 0; i < count * 16; ++i) {
        hash = (hash ^ words[i]) * 1099511628211ull;
    }

    return hash;
}

// A free entry with room for `size` bytes, or NULL.
static struct SkinCacheEntry* skinCacheClaim(unsigned size) {
    for (int tried = 0; tried < SKIN_CACHE_SIZE; ++tried) {
        struct SkinCacheEntry* entry = &sSkinCache[sSkinCacheNext];
        sSkinCacheNext = (sSkinCacheNext + 1) % SKIN_CACHE_SIZE;

        if (entry->skinned && entry->frame == sFrame) {
            continue;
        }

        if (entry->capacity < size) {
            free(entry->skinned);
            entry->skinned = memalign(64, size);
            entry->capacity = entry->skinned ? size : 0;
            entry->part = NULL;
        }

        return entry->skinned ? entry : NULL;
    }

    return NULL;
}

// `bones` is the pose to read and `pose` its hash.
static void drawPartTwoBones(const struct PspModel* model, const struct PspModelPart* part, const ScePspFMatrix4* bones, unsigned long long pose, const ScePspFMatrix4* projectionView, const ScePspFVector3* vertexScale) {
    for (int i = 0; i < SKIN_CACHE_SIZE; ++i) {
        struct SkinCacheEntry* entry = &sSkinCache[i];

        if (entry->part == part && entry->pose == pose) {
            struct PspModelPart flattened = *part;
            flattened.vertices = entry->skinned;
            flattened.vertexBones = NULL;
            entry->frame = sFrame;
            pspProfileCount(PspProfileSkinReused, 1);
            sSkinnedDraw = 1;
            drawPartClipped(&flattened, entry->skinned, NULL, projectionView, vertexScale);
            sSkinnedDraw = 0;
            return;
        }
    }

    unsigned long long skinStart = pspProfileDetailNow();

    // Both layouts are the same size, which static asserts in psp_vertex.h pin.
    unsigned size = sizeof(struct PspVertexNormal) * part->vertexCount;
    struct SkinCacheEntry* entry = skinCacheClaim(size);
    struct PspVertexNormal* skinned = entry ? entry->skinned : cpuPoolAlloc(size);
    const int pooled = !entry && skinned != NULL;

    if (!entry && !pooled) {
        skinned = sceGuGetMemory(size);
    }

    const int isColor = part->vertexFormat == PSP_VERTEX_FORMAT_COLOR;

    // Each bone's world matrix, worked out the first time a vertex needs it.
    static ScePspFMatrix4 worlds[PSP_MAX_BONES];
    static unsigned char known[PSP_MAX_BONES];
    ScePspFMatrix4 identity;
    gumLoadIdentity(&identity);

    for (int b = 0; b < PSP_MAX_BONES; ++b) {
        known[b] = 0;
    }

    memcpy(skinned, part->vertices, size);

    // The bone in VFPU M0; computing a world matrix goes through gum, which uses
    // M0, so reload after.
    const ScePspFMatrix4* loaded = NULL;
    float position[4] __attribute__((aligned(16)));
    float normal[4] __attribute__((aligned(16)));

    for (unsigned short i = 0; i < part->vertexCount; ++i) {
        // A part on one bone has no table and every vertex takes its bone.
        unsigned char boneIndex = part->vertexBones
            ? part->vertexBones[i]
            : (part->boneIndex < 0 ? 255 : (unsigned char)part->boneIndex);
        const ScePspFMatrix4* bone = &identity;

        if (boneIndex != 255 && boneIndex < model->boneCount) {
            if (!known[boneIndex]) {
                known[boneIndex] = boneWorldMatrix(model, bones, boneIndex, &worlds[boneIndex]) ? 1 : 2;
            }

            if (known[boneIndex] == 1) {
                bone = &worlds[boneIndex];
            }
        }

        if (bone != loaded) {
            vfpuLoadMatrix(bone);
            loaded = bone;
        }

        struct PspVertexNormal* v = &skinned[i];

        vfpuTransformPoint(v->x, v->y, v->z, position);
        v->x = roundToShort(position[0]);
        v->y = roundToShort(position[1]);
        v->z = roundToShort(position[2]);

        if (!isColor) {
            // Scaling after is the same; the transform is linear.
            vfpuTransformDirection(v->nx, v->ny, v->nz, normal);
            v->nx = roundToNormal(normal[0] * (1.0f / 127.0f));
            v->ny = roundToNormal(normal[1] * (1.0f / 127.0f));
            v->nz = roundToNormal(normal[2] * (1.0f / 127.0f));
        }
    }

    void* ge = pooled ? geCopy(skinned, size) : skinned;

    if (entry) {
        sceKernelDcacheWritebackRange(skinned, size);
        entry->part = part;
        entry->pose = pose;
        entry->frame = sFrame;
    }

    pspProfileCount(PspProfileSkinned, 1);
    pspProfileDetailAdd(PspProfileSkin, skinStart);

    struct PspModelPart flattened = *part;
    flattened.vertices = skinned;
    flattened.vertexBones = NULL;

    // No exit-plane test: the plane is in part space, which skinned vertices
    // are not.
    sSkinnedDraw = 1;
    drawPartClipped(&flattened, ge, NULL, projectionView, vertexScale);
    sSkinnedDraw = 0;
}

// The second half of (TEXEL0 - PRIMITIVE) * SHADE_ALPHA + PRIMITIVE: the part
// is drawn again as the primitive colour at 1 - alpha over its texel pass,
// giving texel * alpha + primitive * (1 - alpha). Same positions, same depths.
// By shade (PSP_FADE_BY_SHADE) it is per channel.
//
// Cached per part: colours plus only the triangles the fade touches.
#define FADE_CACHE_SIZE 256

struct FadeOverlay {
    const struct PspModelPart* part;
    const struct PspMaterial* material;
    struct PspModelPart overlay;
};

static struct FadeOverlay sFadeCache[FADE_CACHE_SIZE];

static void fadeOverlayColors(const struct PspModelPart* part, const struct PspMaterial* material, struct PspVertexColor* overlay) {
    const int byShade = material->fadeToPrimitive == PSP_FADE_BY_SHADE;
    const int alphaInColor = part->vertexFormat == PSP_VERTEX_FORMAT_COLOR;
    unsigned int primitive = material->primitiveColor & 0x00FFFFFF;

    for (unsigned short i = 0; i < part->vertexCount; ++i) {
        if (byShade) {
            const struct PspVertexColor* source = &((const struct PspVertexColor*)part->vertices)[i];
            unsigned int color = 0xFF000000;

            for (int shift = 0; shift < 24; shift += 8) {
                unsigned int shade = (source->color >> shift) & 0xFF;
                unsigned int prim = (primitive >> shift) & 0xFF;
                color |= ((prim * (255 - shade) + 127) / 255) << shift;
            }

            overlay[i].u = source->u;
            overlay[i].v = source->v;
            overlay[i].color = color;
            overlay[i].x = source->x;
            overlay[i].y = source->y;
            overlay[i].z = source->z;
        } else if (alphaInColor) {
            overlay[i] = ((const struct PspVertexColor*)part->vertices)[i];
            overlay[i].color = ((255 - (overlay[i].color >> 24)) << 24) | primitive;
        } else {
            const struct PspVertexNormal* source = &((const struct PspVertexNormal*)part->vertices)[i];
            overlay[i].u = source->u;
            overlay[i].v = source->v;
            overlay[i].color = ((unsigned int)(255 - source->alpha) << 24) | primitive;
            overlay[i].x = source->x;
            overlay[i].y = source->y;
            overlay[i].z = source->z;
        }

        overlay[i].padding = 0;
    }
}

// Whether the overlay adds anything at this vertex.
static int fadeOverlayShows(const struct PspVertexColor* vertex, int byShade) {
    return byShade ? (vertex->color & 0x00FFFFFF) != 0 : (vertex->color >> 24) != 0;
}

// The cached overlay, or NULL when full; indexCount 0 means nothing to draw.
static const struct PspModelPart* fadeOverlayFor(const struct PspModelPart* part, const struct PspMaterial* material) {
    // Generated parts only; frame-built ones reuse addresses.
    const char* vertices = (const char*)part->vertices;
    const char* key = (const char*)part->indices;

    if (key < _fdata || key >= _edata || vertices < _fdata || vertices >= _edata) {
        return NULL;
    }

    unsigned slot = ((unsigned)part >> 4) % FADE_CACHE_SIZE;

    for (int probe = 0; probe < 8; ++probe, slot = (slot + 1) % FADE_CACHE_SIZE) {
        struct FadeOverlay* entry = &sFadeCache[slot];

        if (entry->part == part && entry->material == material) {
            return &entry->overlay;
        }

        if (entry->part) {
            continue;
        }

        struct PspVertexColor* colors = memalign(16, sizeof(struct PspVertexColor) * part->vertexCount);
        unsigned short* indices = memalign(16, sizeof(unsigned short) * part->indexCount);

        if (!colors || !indices) {
            free(colors);
            free(indices);
            return NULL;
        }

        fadeOverlayColors(part, material, colors);

        const int byShade = material->fadeToPrimitive == PSP_FADE_BY_SHADE;
        unsigned short count = 0;

        for (unsigned short t = 0; t + 2 < part->indexCount; t += 3) {
            const unsigned short* tri = &part->indices[t];

            if (fadeOverlayShows(&colors[tri[0]], byShade) || fadeOverlayShows(&colors[tri[1]], byShade) ||
                fadeOverlayShows(&colors[tri[2]], byShade)) {
                indices[count++] = tri[0];
                indices[count++] = tri[1];
                indices[count++] = tri[2];
            }
        }

        // The GE reads these from memory, past the cache.
        sceKernelDcacheWritebackRange(colors, sizeof(struct PspVertexColor) * part->vertexCount);
        sceKernelDcacheWritebackRange(indices, sizeof(unsigned short) * part->indexCount);

        entry->part = part;
        entry->material = material;
        entry->overlay = *part;
        entry->overlay.vertices = colors;
        entry->overlay.indices = indices;
        entry->overlay.indexCount = count;
        entry->overlay.vertexFormat = PSP_VERTEX_FORMAT_COLOR;
        return &entry->overlay;
    }

    return NULL;
}

static void drawFadeToPrimitive(const struct PspModelPart* part, const struct PspMaterial* material, const ScePspFMatrix4* projectionView, const ScePspFVector3* vertexScale) {
    const int byShade = material->fadeToPrimitive == PSP_FADE_BY_SHADE;
    const struct PspModelPart* cached = fadeOverlayFor(part, material);
    struct PspModelPart copy;
    const void* ge;

    if (cached) {
        if (!cached->indexCount) {
            return;
        }

        copy = *cached;
        ge = cached->vertices;
    } else {
        unsigned size = sizeof(struct PspVertexColor) * part->vertexCount;
        struct PspVertexColor* overlay = cpuPoolAlloc(size);

        if (!overlay) {
            return;
        }

        fadeOverlayColors(part, material, overlay);
        copy = *part;
        copy.vertices = overlay;
        copy.vertexFormat = PSP_VERTEX_FORMAT_COLOR;
        ge = geCopy(overlay, size);
    }

    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_FOG);
    sceGuEnable(GU_BLEND);

    if (byShade) {
        sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF);
    } else {
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    }

    sceGuDepthMask(1);
    sceGuColor(0xFFFFFFFF);

    // The same positions as the part's, so its bounding sphere holds.
    drawPartClipped(&copy, ge, part, projectionView, vertexScale);

    // Restore the material; the next part may reuse it without a bind.
    pspMaterialForgetState();
    materialBindState(material, 0);
}

void pspModelDrawSkinned(const struct PspModel* model, const void* boneMatrices) {
    const ScePspFMatrix4* bones = (const ScePspFMatrix4*)boneMatrices;

    // The GE reads 16 bit positions as fractions of 32768; scale back to the
    // pipeline's SCENE_SCALE units. Last, so it applies before the bone.
    static const ScePspFVector3 vertexScale = {
        PSP_VERTEX_POSITION_UNSCALE,
        PSP_VERTEX_POSITION_UNSCALE,
        PSP_VERTEX_POSITION_UNSCALE,
    };

    const int skinned = bones && model->boneParent && model->boneCount;
    unsigned long long pose = 0;

    if (skinned) {
        bones = poseCached(bones, model->boneCount);
        pose = poseHash(bones, model->boneCount);
    }

    unsigned long long matrixStart = pspProfileDetailNow();

    // The clipper's pre-model transform, read from sceGum only when it changes.
    if (!sProjectionViewValid) {
        ScePspFMatrix4 projection;
        ScePspFMatrix4 view;

        sceGumMatrixMode(GU_PROJECTION);
        sceGumStoreMatrix(&projection);
        sceGumMatrixMode(GU_VIEW);
        sceGumStoreMatrix(&view);
        gumMultMatrix(&sProjectionView, &projection, &view);
        sProjectionViewValid = 1;
    }

    const ScePspFMatrix4 projectionView = sProjectionView;

    sceGumMatrixMode(GU_MODEL);

    pspProfileDetailAdd(PspProfileMatrix, matrixStart);

    int modelHasJoints = 0;
    static ScePspFMatrix4 worlds[PSP_MAX_BONES];
    unsigned char worldKnown[PSP_MAX_BONES] = {0};

    for (unsigned short i = 0; skinned && i < model->partCount; ++i) {
        if (model->parts[i].vertexBones) {
            modelHasJoints = 1;
            break;
        }
    }

    for (unsigned short i = 0; i < model->partCount; ++i) {
        const struct PspModelPart* part = &model->parts[i];

        // Leaving a part out is better than writing past the list's end.
        if (displayPspListRemaining() < PSP_LIST_RESERVE) {
            sPendingMaterial = NULL;
            return;
        }

        matrixStart = pspProfileDetailNow();
        sceGumPushMatrix();
        pspProfileDetailAdd(PspProfileMatrix, matrixStart);

        // Parts without a material keep the scene's, as on the N64.
        sPendingMaterial = part->material;

        // Models with joints are skinned entirely on the CPU: a joint vertex shared
        // with a GE-transformed rigid part rounded differently and left seams.
        if (skinned && (part->vertexBones || modelHasJoints)) {
            drawPartTwoBones(model, part, bones, pose, &projectionView, &vertexScale);
            sceGumPopMatrix();
            continue;
        }

        if (skinned && part->boneIndex >= 0 && part->boneIndex < PSP_MAX_BONES) {
            matrixStart = pspProfileDetailNow();

            // Once per bone per draw, not per part (the gun is 41 parts on 9 bones).
            if (!worldKnown[part->boneIndex]) {
                worldKnown[part->boneIndex] = boneWorldMatrix(model, bones, part->boneIndex, &worlds[part->boneIndex]) ? 1 : 2;
            }

            if (worldKnown[part->boneIndex] == 1) {
                sceGumMultMatrix(&worlds[part->boneIndex]);
            }

            pspProfileDetailAdd(PspProfileMatrix, matrixStart);
        }

        // Materials that ignore vertex colour have their colour baked into the
        // vertices (the GE replaces the material colour with it). A runtime colour
        // (gun indicator, fizzle tint) is drawn from a recoloured copy.
        const struct PspMaterial* material = part->material ? part->material : sBoundMaterial;

        if (material && material->cullBack == PSP_CULL_BOTH) {
            sceGumPopMatrix();
            continue;
        }

        if (material && material->shadePlusOne &&
            part->vertexFormat == PSP_VERTEX_FORMAT_COLOR && part->vertexCount) {
            // texel * (1 + shade) = 2 * texel * ((1 + shade) / 2): the
            // doubling is the material's, the halfway raise is this.
            unsigned long long copyStart = pspProfileDetailNow();
            pspProfileCount(PspProfileCopies, 1);
            unsigned size = sizeof(struct PspVertexColor) * part->vertexCount;
            struct PspVertexColor* raised = cpuPoolAlloc(size);
            const int pooled = raised != NULL;

            if (!pooled) {
                raised = sceGuGetMemory(size);
            }

            const struct PspVertexColor* source = part->vertices;

            for (unsigned short v = 0; v < part->vertexCount; ++v) {
                unsigned int c = source[v].color;
                raised[v] = source[v];
                raised[v].color = (c & 0xFF000000) |
                    (((255 + ((c >> 16) & 0xFF)) >> 1) << 16) |
                    (((255 + ((c >> 8) & 0xFF)) >> 1) << 8) |
                    ((255 + (c & 0xFF)) >> 1);
            }

            struct PspModelPart copy = *part;
            copy.vertices = raised;
            const void* ge = pooled ? geCopy(raised, size) : raised;
            pspProfileDetailAdd(PspProfileCopy, copyStart);
            // The same positions as the part's, so its bounding sphere holds.
            drawPartClipped(&copy, ge, part, &projectionView, &vertexScale);
        } else if (material &&
            material->fragmentSource != PSP_COLOR_SOURCE_SHADE &&
            part->vertexFormat == PSP_VERTEX_FORMAT_COLOR &&
            part->vertexCount &&
            ((const struct PspVertexColor*)part->vertices)[0].color != material->primitiveColor) {
            unsigned long long copyStart = pspProfileDetailNow();
            pspProfileCount(PspProfileCopies, 1);
            unsigned size = sizeof(struct PspVertexColor) * part->vertexCount;
            struct PspVertexColor* recolored = cpuPoolAlloc(size);
            const int pooled = recolored != NULL;

            if (!pooled) {
                recolored = sceGuGetMemory(size);
            }

            memcpy(recolored, part->vertices, size);

            for (unsigned short v = 0; v < part->vertexCount; ++v) {
                recolored[v].color = material->primitiveColor;
            }

            struct PspModelPart copy = *part;
            copy.vertices = recolored;
            const void* ge = pooled ? geCopy(recolored, size) : recolored;
            pspProfileDetailAdd(PspProfileCopy, copyStart);
            drawPartClipped(&copy, ge, part, &projectionView, &vertexScale);
        } else {
            drawPartClipped(part, part->vertices, part, &projectionView, &vertexScale);

            // By shade needs colour vertices; by alpha takes either format.
            if (material && material->fadeToPrimitive &&
                (material->fadeToPrimitive == PSP_FADE_BY_ALPHA || part->vertexFormat == PSP_VERTEX_FORMAT_COLOR) &&
                !(skinned && part->boneIndex >= 0)) {
                // The unscale is still on the model matrix; the overlay uses the same path.
                sceGumPopMatrix();
                sceGumPushMatrix();
                drawFadeToPrimitive(part, material, &projectionView, &vertexScale);
            }
        }

        matrixStart = pspProfileDetailNow();
        sceGumPopMatrix();
        pspProfileDetailAdd(PspProfileMatrix, matrixStart);
    }

    sPendingMaterial = NULL;
}

void pspModelDraw(const struct PspModel* model) {
    pspModelDrawSkinned(model, NULL);
}

struct PspModel* pspModelBuild(
    struct RenderState* renderState,
    const void* vertices,
    unsigned short vertexCount,
    const unsigned short* indices,
    unsigned short indexCount,
    unsigned int vertexFormat,
    const struct PspMaterial* material
) {
    // One allocation, so a returned model always has its part.
    struct PspModel* model = renderStateRequestMemory(
        renderState, sizeof(struct PspModel) + sizeof(struct PspModelPart));

    if (!model) {
        return NULL;
    }

    struct PspModelPart* part = (struct PspModelPart*)(model + 1);

    part->vertices = vertices;
    part->indices = indices;
    part->indexCount = indexCount;
    part->vertexCount = vertexCount;
    part->vertexFormat = vertexFormat;
    part->material = material;
    // Frame-built geometry is already in draw space and unskinned.
    part->boneIndex = -1;
    part->secondBoneIndex = -1;
    part->vertexBones = NULL;

    model->parts = part;
    model->partCount = 1;
    model->boneParent = NULL;
    model->boneCount = 0;

    return model;
}
