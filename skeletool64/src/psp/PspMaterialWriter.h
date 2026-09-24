#ifndef _PSP_MATERIAL_WRITER_H
#define _PSP_MATERIAL_WRITER_H

#include <map>
#include <memory>
#include <string>

#include "../CFileDefinition.h"
#include "../materials/Material.h"
#include "../materials/TextureDefinition.h"

// Reduces a material to what the GU can express, for the mesh writer and
// the material list generator. The caller's two maps make each texture and
// material emitted once.

// A texture is emitted once per way it is drawn: as it is, with its colour
// thrown away for a material that only takes its alpha, or inverted for a
// material whose blend runs the other way round from GU_TFX_BLEND.
typedef std::map<std::pair<std::shared_ptr<TextureDefinition>, int>, std::string> PspEmittedTextures;

#define PSP_TEXTURE_PLAIN       0
#define PSP_TEXTURE_ALPHA_ONLY  1
#define PSP_TEXTURE_INVERTED    2

std::string usePspTexture(
    PspEmittedTextures& emitted,
    Material* material,
    int variant,
    CFileDefinition& fileDefinition,
    const std::string& fileSuffix
);

// Mirrors PSP_COLOR_SOURCE_* in src/graphics/psp/psp_model.h.
enum class PspColorSource {
    Shade = 0,
    Primitive = 1,
    Environment = 2,
    // The combiner's constant 1: white, and nothing sets it at run time.
    White = 3,
};

// The RDP combiner reduced to the GE's single texture stage: which function,
// whether the texture has to be inverted for it, what the fragment colour
// stands for, and what GU_TFX_BLEND's second colour stands for.
struct PspCombine {
    const char* function;
    bool invertTexture;
    PspColorSource fragment;
    PspColorSource blendColor;
};

PspCombine pspAnalyzeCombine(const MaterialState& state);
std::string pspColorOf(const MaterialState& state, PspColorSource source);

// The tile TEXEL0 reads: the one gSPTexture names, which is not always tile 0.
// The ball catcher's materials load three textures and each picks one --
// read as tile 0, its ring and blue parts all drew the back texture.
inline const TileState& pspTexelTile(const MaterialState& state) {
    int tile = state.textureState.isOn ? state.textureState.tile : 0;
    return state.tiles[(tile >= 0 && tile < MAX_TILE_COUNT) ? tile : 0];
}

std::string usePspMaterial(
    std::map<Material*, std::string>& emitted,
    PspEmittedTextures& emittedTextures,
    Material* material,
    const std::string& materialName,
    const MaterialState& defaultState,
    CFileDefinition& fileDefinition,
    const std::string& fileSuffix
);

#endif
