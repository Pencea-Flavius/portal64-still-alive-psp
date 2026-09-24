#include <cstdio>
#include "PspMaterialWriter.h"

#include <iomanip>
#include <sstream>

#include "../definitions/DataChunk.h"

#define RDP_Z_CMP       0x0010
#define RDP_Z_UPD       0x0020
#define RDP_FORCE_BL    0x4000
#define RDP_ZMODE_MASK  0x0c00
#define RDP_ZMODE_DEC   0x0c00
#define RDP_CVG_X_ALPHA 0x1000

// Where GBL_c1() and GBL_c2() put the formula's second colour (the "m" in
// p * a + m * b), and the value that means the colour already in memory.
#define RDP_BLEND_C1_M2A_SHIFT  22
#define RDP_BLEND_C2_M2A_SHIFT  20
#define RDP_BL_CLR_MEM          1
#define RDP_BL_CLR_FOG          3

// Mirroring is baked into the texture at twice the size and repeats; the GU
// has no mirrored wrap. Otherwise follow the RDP's clamp bit.
const char* pspTextureWrap(const TileState& tile, bool isS) {
    const TextureCoordinateState& coord = isS ? tile.sCoord : tile.tCoord;

    return (coord.mirror || coord.wrap) ? "GU_REPEAT" : "GU_CLAMP";
}

const char* pspTextureFilter(const MaterialState& state) {
    return state.textureFilter == TextureFilter::Point ? "GU_NEAREST" : "GU_LINEAR";
}

// The RDP computes (a - b) * c + d. The texel alone in d is REPLACE;
// anything multiplying it (shade or primitive in c) is MODULATE.
const char* pspTextureFunction(const MaterialState& state) {
    if (!state.hasCombineMode) {
        return "GU_TFX_MODULATE";
    }

    bool textureIsWholeColor = state.cycle1Combine.color[3] == ColorCombineSource::Texel0;
    bool textureIsMultiplied = state.cycle1Combine.color[2] == ColorCombineSource::Texel0;

    return (textureIsWholeColor && !textureIsMultiplied)
        ? "GU_TFX_REPLACE"
        : "GU_TFX_MODULATE";
}

const char* pspTextureComponent(const MaterialState& state) {
    if (!state.hasCombineMode) {
        return "GU_TCC_RGBA";
    }

    // The texel's alpha counts anywhere in the alpha combiner, not only in d:
    // the font's is [TEXEL0, 0, ENVIRONMENT, 0].
    for (int i = 0; i < 4; ++i) {
        if (state.cycle1Combine.alpha[i] == AlphaCombineSource::Texture0Alpha) {
            return "GU_TCC_RGBA";
        }
    }

    // And in the second cycle (the gun's flare: [0, 0, 0, TEXEL1]).
    if (state.cycleType == CycleType::_2Cycle) {
        for (int i = 0; i < 4; ++i) {
            if (state.cycle2Combine.alpha[i] == AlphaCombineSource::Texture0Alpha ||
                state.cycle2Combine.alpha[i] == AlphaCombineSource::Texture1Alpha) {
                return "GU_TCC_RGBA";
            }
        }
    }

    return "GU_TCC_RGB";
}

bool pspUsesLighting(const MaterialState& state) {
    return (state.geometryModes.flags & (uint64_t)GeometryMode::G_LIGHTING) != 0;
}

// Whether the combiner reads a texel at all. The observation glass sets a
// tile but draws G_CC_PRIMITIVE.
bool pspCombinerReadsTexture(const MaterialState& state) {
    if (!state.hasCombineMode) {
        return true;
    }

    auto reads = [](const ColorCombineMode& mode) {
        for (int i = 0; i < 4; ++i) {
            if (mode.color[i] == ColorCombineSource::Texel0 ||
                mode.color[i] == ColorCombineSource::Texel1 ||
                mode.color[i] == ColorCombineSource::Texture0Alpha ||
                mode.color[i] == ColorCombineSource::Texture1Alpha ||
                mode.alpha[i] == AlphaCombineSource::Texture0Alpha ||
                mode.alpha[i] == AlphaCombineSource::Texture1Alpha) {
                return true;
            }
        }

        return false;
    };

    return reads(state.cycle1Combine) ||
        (state.cycleType == CycleType::_2Cycle && reads(state.cycle2Combine));
}

// Whether the combiner reads the vertex colour. On the GE a vertex colour
// always replaces the material's, so for materials that ignore shade the
// constant colour is written into the vertices (see generatePspMesh()).
bool pspCombinerReadsShade(const MaterialState& state) {
    if (!state.hasCombineMode) {
        return true;
    }

    auto reads = [](const ColorCombineMode& mode) {
        for (int i = 0; i < 4; ++i) {
            if (mode.color[i] == ColorCombineSource::ShadeColor ||
                mode.color[i] == ColorCombineSource::ShadedAlpha ||
                mode.alpha[i] == AlphaCombineSource::ShadedAlpha) {
                return true;
            }
        }

        return false;
    };

    return reads(state.cycle1Combine) ||
        (state.cycleType == CycleType::_2Cycle && reads(state.cycle2Combine));
}

// Colour ignores the texel but alpha reads it (signage overlays: colour
// PRIMITIVE, alpha TEXEL0). MODULATE would tint by the texel, so these get a
// white texture.
bool pspTextureIsAlphaOnly(const MaterialState& state) {
    if (!state.hasCombineMode) {
        return false;
    }

    bool colorReads = false;
    bool alphaReads = false;

    auto scan = [&](const ColorCombineMode& mode) {
        for (int i = 0; i < 4; ++i) {
            colorReads = colorReads ||
                mode.color[i] == ColorCombineSource::Texel0 ||
                mode.color[i] == ColorCombineSource::Texel1 ||
                mode.color[i] == ColorCombineSource::Texture0Alpha ||
                mode.color[i] == ColorCombineSource::Texture1Alpha;
            alphaReads = alphaReads ||
                mode.alpha[i] == AlphaCombineSource::Texture0Alpha ||
                mode.alpha[i] == AlphaCombineSource::Texture1Alpha;
        }
    };

    scan(state.cycle1Combine);

    if (state.cycleType == CycleType::_2Cycle) {
        scan(state.cycle2Combine);
    }

    return alphaReads && !colorReads;
}

// Second cycle COMBINED * SHADE + COMBINED = texel * (1 + shade) (portal
// particles, muzzle flash). On the GE: 2 * texel * ((1 + shade) / 2),
// MODULATE doubled with shade raised halfway (see pspModelDrawSkinned()).
bool pspCombinerShadePlusOne(const MaterialState& state) {
    if (!state.hasCombineMode || state.cycleType != CycleType::_2Cycle) {
        return false;
    }

    const ColorCombineSource* c = state.cycle2Combine.color;

    return c[0] == ColorCombineSource::Combined &&
        c[1] == ColorCombineSource::_0 &&
        c[2] == ColorCombineSource::ShadeColor &&
        c[3] == ColorCombineSource::Combined;
}

// Second cycle COMBINED + COMBINED doubles the colour (the turret):
// GU_FRAGMENT_2X.
bool pspCombinerDoubles(const MaterialState& state) {
    if (!state.hasCombineMode || state.cycleType != CycleType::_2Cycle) {
        return false;
    }

    const ColorCombineSource* c = state.cycle2Combine.color;

    return c[0] == ColorCombineSource::_1 &&
        c[1] == ColorCombineSource::_0 &&
        c[2] == ColorCombineSource::Combined &&
        c[3] == ColorCombineSource::Combined;
}

std::string pspPrimitiveColor(const MaterialState& state) {
    unsigned r = 0xFF, g = 0xFF, b = 0xFF, a = 0xFF;

    if (state.usePrimitiveColor) {
        r = state.primitiveColor.r;
        g = state.primitiveColor.g;
        b = state.primitiveColor.b;
        a = state.primitiveColor.a;
    }

    // Two tone textures have their colours baked in (see
    // TextureDefinition::GeneratePspDefinitions()); don't multiply again.
    const std::shared_ptr<TextureDefinition>& texture = pspTexelTile(state).texture;

    if (texture && texture->HasEffect(TextureDefinitionEffect::TwoToneGrayscale)) {
        r = g = b = 0xFF;
    }

    std::ostringstream packed;
    packed << "0x" << std::hex << std::uppercase
           << ((a << 24) | (b << 16) | (g << 8) | r);
    return packed.str();
}

static bool colorSourceOf(ColorCombineSource source, PspColorSource& out) {
    switch (source) {
        case ColorCombineSource::ShadeColor:        out = PspColorSource::Shade; return true;
        case ColorCombineSource::PrimitiveColor:    out = PspColorSource::Primitive; return true;
        case ColorCombineSource::EnvironmentColor:  out = PspColorSource::Environment; return true;
        case ColorCombineSource::_1:                out = PspColorSource::White; return true;
        default: return false;
    }
}

// (a - b) * c + d with the texel as c and b == d is a blend from b to a (the
// sign's LCD: [SHADE, PRIMITIVE, TEXEL0, PRIMITIVE]). GU_TFX_BLEND is
// fragment * (1 - t) + env * t; when a is shade it must be the fragment, so
// the blend is reversed and the texture inverted.
PspCombine pspAnalyzeCombine(const MaterialState& state) {
    PspCombine result;
    result.function = pspTextureFunction(state);
    result.invertTexture = false;
    result.blendColor = PspColorSource::Primitive;

    const ColorCombineSource* c = state.cycle1Combine.color;

    if (pspCombinerReadsShade(state)) {
        result.fragment = PspColorSource::Shade;
    } else if (state.hasCombineMode && c[3] == ColorCombineSource::EnvironmentColor && c[2] == ColorCombineSource::_0) {
        // Environment alone (the sign's dark LCD).
        result.fragment = PspColorSource::Environment;
    } else {
        result.fragment = PspColorSource::Primitive;
    }

    // Two tone textures already have their blend baked in.
    const std::shared_ptr<TextureDefinition>& texture = pspTexelTile(state).texture;
    bool twoTone = texture && texture->HasEffect(TextureDefinitionEffect::TwoToneGrayscale);

    PspColorSource a;
    PspColorSource b;

    // The factor may be the texel's alpha, which equals its colour for intensity
    // textures (the projectile's glow). Other formats are left alone.
    bool texelFactor = c[2] == ColorCombineSource::Texel0 ||
        (c[2] == ColorCombineSource::Texture0Alpha && texture && texture->Format() == G_IM_FMT::G_IM_FMT_I);

    if (state.hasCombineMode && !twoTone &&
        texelFactor && c[1] == c[3] &&
        colorSourceOf(c[0], a) && colorSourceOf(c[1], b) && a != b &&
        b != PspColorSource::White) {
        result.function = "GU_TFX_BLEND";

        if (a == PspColorSource::Shade) {
            result.invertTexture = true;
            result.fragment = PspColorSource::Shade;
            result.blendColor = b;
        } else {
            result.fragment = b;
            result.blendColor = a;
        }
    }

    if (pspCombinerShadePlusOne(state)) {
        result.function = "GU_TFX_MODULATE";
        result.fragment = PspColorSource::Shade;
    }

    return result;
}

std::string pspColorOf(const MaterialState& state, PspColorSource source) {
    if (source == PspColorSource::White) {
        return "0xFFFFFFFF";
    }

    if (source != PspColorSource::Environment) {
        return pspPrimitiveColor(state);
    }

    PixelRGBAu8 env = state.useEnvColor ? state.envColor : PixelRGBAu8(0xFF, 0xFF, 0xFF, 0xFF);

    std::ostringstream packed;
    packed << "0x" << std::hex << std::uppercase
           << (((unsigned)env.a << 24) | ((unsigned)env.b << 16) | ((unsigned)env.g << 8) | (unsigned)env.r);
    return packed.str();
}

// Emits each texture once and returns its name, or "" if none.
std::string usePspTexture(
    PspEmittedTextures& emitted,
    Material* material,
    int variant,
    CFileDefinition& fileDefinition,
    const std::string& fileSuffix
) {
    if (!material) {
        return "";
    }

    std::shared_ptr<TextureDefinition> texture = pspTexelTile(material->mState).texture;

    if (!pspTexelTile(material->mState).isOn || !texture) {
        return "";
    }

    auto existing = emitted.find(std::make_pair(texture, variant));

    if (existing != emitted.end()) {
        return existing->second;
    }

    const TileState& tile = pspTexelTile(material->mState);
    bool mirrorS = tile.sCoord.mirror;
    bool mirrorT = tile.tCoord.mirror;

    std::string baseName = texture->Name();

    if (variant == PSP_TEXTURE_ALPHA_ONLY) {
        baseName += "_alpha";
    } else if (variant == PSP_TEXTURE_INVERTED) {
        baseName += "_inverted";
    }
    std::string dataName = fileDefinition.GetUniqueName(baseName);

    bool swizzled = false;
    std::string format;
    std::unique_ptr<FileDefinition> clut;
    auto levels = texture->GeneratePspDefinitions(
        dataName, fileSuffix, mirrorS, mirrorT,
        variant == PSP_TEXTURE_ALPHA_ONLY, variant == PSP_TEXTURE_INVERTED, &swizzled, &format, &clut);
    unsigned levelCount = levels.size();

    std::string clutName = "0";

    if (clut) {
        clutName = dataName + "_clut";
        fileDefinition.AddDefinition(std::move(clut));
    }

    std::unique_ptr<StructureDataChunk> levelList(new StructureDataChunk());

    for (unsigned level = 0; level < levelCount; ++level) {
        levelList->AddPrimitive(level == 0 ? dataName : dataName + "_mip" + std::to_string(level));
        fileDefinition.AddDefinition(std::move(levels[level]));
    }

    std::string levelsName = fileDefinition.AddDataDefinition(
        baseName + "_levels", "const void*", true, fileSuffix, std::move(levelList));

    std::unique_ptr<StructureDataChunk> descriptor(new StructureDataChunk());
    descriptor->AddPrimitive(levelsName);
    descriptor->AddPrimitive(levelCount);
    // The size actually bound (mirror doubled, padded to a power of two); UVs
    // and 2D texel coordinates are measured against it.
    descriptor->AddPrimitive(TextureDefinition::PspPaddedSize(texture->Width() * (mirrorS ? 2 : 1)));
    descriptor->AddPrimitive(TextureDefinition::PspPaddedSize(texture->Height() * (mirrorT ? 2 : 1)));
    descriptor->AddPrimitive(format);
    descriptor->AddPrimitive(swizzled ? 1 : 0);
    descriptor->AddPrimitive(clutName);

    std::string descriptorName = fileDefinition.AddDataDefinition(
        baseName + "_texture", "struct PspTexture", false, fileSuffix, std::move(descriptor));

    emitted[std::make_pair(texture, variant)] = descriptorName;

    return descriptorName;
}

// Emits each material once, with the RDP state reduced to what the GU applies.
std::string usePspMaterial(
    std::map<Material*, std::string>& emitted,
    PspEmittedTextures& emittedTextures,
    Material* material,
    const std::string& materialName,
    const MaterialState& defaultState,
    CFileDefinition& fileDefinition,
    const std::string& fileSuffix
) {
    auto existing = emitted.find(material);

    if (existing != emitted.end()) {
        return existing->second;
    }

    // Materials without a render mode inherit it on the N64, so apply over the
    // default material.
    MaterialState state = defaultState;

    if (material) {
        applyMaterial(material->mState, state);
    }

    PspCombine combine = pspAnalyzeCombine(state);

    int variant = PSP_TEXTURE_PLAIN;

    if (combine.invertTexture) {
        variant = PSP_TEXTURE_INVERTED;
    } else if (pspTextureIsAlphaOnly(state)) {
        variant = PSP_TEXTURE_ALPHA_ONLY;
    }

    std::string textureName = pspCombinerReadsTexture(state)
        ? usePspTexture(emittedTextures, material, variant, fileDefinition, fileSuffix)
        : std::string();

    bool depthTest;
    bool depthWrite;
    bool blend;
    bool decal = false;
    // Alpha test threshold for cut-outs, 0 for none.
    int alphaTest = 0;

    if (state.hasRenderMode) {
        // OR both cycles, as the RDP does: G_RM_PASS carries no flags, the depth
        // and blend bits are in the second.
        int renderMode = state.cycle1RenderMode.data | state.cycle2RenderMode.data;

        depthTest = (renderMode & RDP_Z_CMP) != 0;
        depthWrite = (renderMode & RDP_Z_UPD) != 0;
        // FORCE_BL only runs the blender; it blends only if m is the memory colour
        // in the last cycle's (p * a + m * b). Bits 20-23 hold m in both GBL_c1 and
        // GBL_c2 layouts, so both are read.
        bool twoCycle = state.cycleType == CycleType::_2Cycle;
        int lastCycle = twoCycle ? state.cycle2RenderMode.data : state.cycle1RenderMode.data;
        int blenderMemory = ((lastCycle >> RDP_BLEND_C1_M2A_SHIFT) & 3) | ((lastCycle >> RDP_BLEND_C2_M2A_SHIFT) & 3);

        blend = (renderMode & RDP_FORCE_BL) != 0 && blenderMemory == RDP_BL_CLR_MEM;

        // G_AC_THRESHOLD and the TEX_EDGE coverage modes both map to the alpha test
        // (metalgrate018 over the red haze).
        if (state.alphaCompare == AlphaCompare::Threshold) {
            alphaTest = state.useBlendColor && state.blendColor.a ? state.blendColor.a : 128;
        } else if ((renderMode & RDP_CVG_X_ALPHA) && !blend) {
            alphaTest = 128;
        }

        // Decals: no depth write; the renderer pulls them towards the camera.
        if ((renderMode & RDP_ZMODE_MASK) == RDP_ZMODE_DEC) {
            depthWrite = false;
            decal = true;
        }
    } else {
        // No render mode: opaque and depth tested, as the game's init sets up.
        depthTest = true;
        depthWrite = true;
        blend = false;
    }

    // pspDecal: a surface lying on another of the same model that the N64's
    // depth keeps apart but 16 bits do not (the ball catcher's lit panels).
    if (material && material->mProperties.count("pspDecal")) {
        decal = true;
    }

    // pspDepthWhereOpaque: a translucent material that still writes depth,
    // alpha tested, for long surfaces whose centre sorts them wrongly.
    if (material && material->mProperties.count("pspDepthWhereOpaque")) {
        depthWrite = true;

        if (!alphaTest) {
            alphaTest = 128;
        }
    }

    std::unique_ptr<StructureDataChunk> descriptor(new StructureDataChunk());
    descriptor->Add(std::unique_ptr<DataChunk>(new StringDataChunk(materialName)));
    // (TEXEL0 - PRIMITIVE) * SHADE_ALPHA + PRIMITIVE fades the texel to the
    // primitive colour by vertex alpha (walls reddening towards the pits). The GE
    // can't combine that, so the texel is drawn plain and the renderer overlays
    // the primitive colour (fadeToPrimitive). Lighting stays off.
    //
    // With SHADE instead of SHADE_ALPHA (metalwall_bts_006a), MODULATE draws
    // texel * shade and the renderer adds primitive * (1 - shade) (mode 2).
    const ColorCombineSource* fade = state.cycle1Combine.color;
    bool fadeShape = state.cycleType != CycleType::_2Cycle &&
        fade[0] == ColorCombineSource::Texel0 &&
        fade[1] == ColorCombineSource::PrimitiveColor &&
        fade[3] == ColorCombineSource::PrimitiveColor;
    bool fadeToPrimitive = fadeShape && fade[2] == ColorCombineSource::ShadedAlpha;
    bool fadeByShade = fadeShape && fade[2] == ColorCombineSource::ShadeColor;

    // The same fade in two cycles: MODULATE, then the fade by alpha.
    const ColorCombineSource* first = state.cycle1Combine.color;
    const ColorCombineSource* second = state.cycle2Combine.color;
    bool fadeBySecondCycle = state.cycleType == CycleType::_2Cycle &&
        first[0] == ColorCombineSource::Texel0 &&
        first[2] == ColorCombineSource::ShadeColor &&
        second[0] == ColorCombineSource::Combined &&
        second[1] == ColorCombineSource::PrimitiveColor &&
        second[2] == ColorCombineSource::ShadedAlpha &&
        second[3] == ColorCombineSource::PrimitiveColor;

    descriptor->AddPrimitive(textureName.empty() ? std::string("0") : "&" + textureName);
    descriptor->AddPrimitive(fadeToPrimitive ? std::string("GU_TFX_REPLACE") : std::string(combine.function));
    descriptor->AddPrimitive(std::string(pspTextureComponent(state)));
    descriptor->AddPrimitive(pspColorOf(state, combine.fragment));
    descriptor->AddPrimitive(std::string(pspTextureFilter(state)));
    descriptor->AddPrimitive(std::string(pspTextureWrap(pspTexelTile(state), true)));
    descriptor->AddPrimitive(std::string(pspTextureWrap(pspTexelTile(state), false)));
    descriptor->AddPrimitive(depthTest ? 1 : 0);
    descriptor->AddPrimitive(depthWrite ? 1 : 0);
    descriptor->AddPrimitive(blend ? 1 : 0);
    descriptor->AddPrimitive(pspUsesLighting(state) && !fadeToPrimitive ? 1 : 0);
    // Both faces culled: an invisible collision/portal surface. The GE culls one
    // face at most, so PSP_CULL_BOTH makes the renderer skip it.
    uint64_t culled = state.geometryModes.flags & ((uint64_t)GeometryMode::G_CULL_BACK | (uint64_t)GeometryMode::G_CULL_FRONT);
    descriptor->AddPrimitive(
        culled == ((uint64_t)GeometryMode::G_CULL_BACK | (uint64_t)GeometryMode::G_CULL_FRONT) ? 2 :
        (culled & (uint64_t)GeometryMode::G_CULL_BACK) ? 1 : 0
    );
    // 3: a decal with a smaller minimum pull (pspDecalLight).
    bool pspDecalLight = decal && material && material->mProperties.count("pspDecalLight");
    descriptor->AddPrimitive(pspDecalLight ? 3 : decal ? 1 : 0);
    descriptor->AddPrimitive((int)combine.fragment);
    descriptor->AddPrimitive((int)combine.blendColor);
    descriptor->AddPrimitive(pspColorOf(state, combine.blendColor));
    descriptor->AddPrimitive((pspCombinerDoubles(state) || pspCombinerShadePlusOne(state)) ? 1 : 0);
    descriptor->AddPrimitive((state.geometryModes.flags & (uint64_t)GeometryMode::G_TEXTURE_GEN) ? 1 : 0);
    descriptor->AddPrimitive(pspCombinerShadePlusOne(state) ? 1 : 0);
    descriptor->AddPrimitive(alphaTest);

    // G_FOG with G_RM_FOG_SHADE_A is GE fog; G_FOG alone (portal trail) only
    // feeds shade alpha.
    uint32_t firstCycle = state.hasRenderMode ? (uint32_t)state.cycle1RenderMode.data : 0;
    bool fogged = (state.geometryModes.flags & (uint64_t)GeometryMode::G_FOG) &&
        state.useFogColor &&
        ((firstCycle >> 30) & 3) == RDP_BL_CLR_FOG;
    char fogColor[16];
    snprintf(fogColor, sizeof(fogColor), "0x%08X", fogged
        ? (0xFF000000u | ((uint32_t)state.fogColor.b << 16) | ((uint32_t)state.fogColor.g << 8) | state.fogColor.r)
        : 0u);
    descriptor->AddPrimitive(std::string(fogColor));
    descriptor->AddPrimitive(fadeToPrimitive || fadeBySecondCycle ? 1 : fadeByShade ? 2 : 0);

    std::string name = fileDefinition.AddDataDefinition(
        materialName + "_material", "struct PspMaterial", false, fileSuffix, std::move(descriptor));

    emitted[material] = name;

    return name;
}
