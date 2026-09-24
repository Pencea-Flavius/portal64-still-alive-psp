#include "MaterialGenerator.h"

#include "../StringUtils.h"
#include "../materials/RenderMode.h"
#include "../psp/PspMaterialWriter.h"

MaterialGenerator::MaterialGenerator(const DisplayListSettings& settings): mSettings(settings) {}


bool MaterialGenerator::ShouldIncludeNode(aiNode* node) {
    return false;
}

#define OPAQUE_ORDER        0
#define DECAL_ORDER         1
#define TRANSPARENT_ORDER   2

int sortOrderForMaterial(const Material& material) {
    // assume opaque
    if (!material.mState.hasRenderMode) {
        return OPAQUE_ORDER;
    }

    if (material.mState.cycle1RenderMode.GetZMode() == ZMODE_DEC ||
        material.mState.cycle2RenderMode.GetZMode() == ZMODE_DEC) {
        return DECAL_ORDER;
    }

    if ((material.mState.cycle1RenderMode.data | material.mState.cycle2RenderMode.data) & FORCE_BL) {
        return TRANSPARENT_ORDER;
    }

    return OPAQUE_ORDER;
}

// Sorted the same way as the N64 list, so a material's index means the same
// thing on either machine and the generated *_INDEX macros stay comparable.
void MaterialGenerator::GeneratePspDefinitions(CFileDefinition& fileDefinition, std::vector<std::shared_ptr<Material>>& materials) {
    fileDefinition.AddHeader("\"graphics/psp/psp_model.h\"");

    PspEmittedTextures emittedTextures;
    std::map<Material*, std::string> emittedMaterials;

    std::unique_ptr<StructureDataChunk> materialList(new StructureDataChunk());

    int index = 0;

    for (auto& entry : materials) {
        MaterialState defaultState = entry->mName == mSettings.mDefaultMaterialName
            ? MaterialState()
            : mSettings.mDefaultMaterialState;

        std::string name = usePspMaterial(
            emittedMaterials,
            emittedTextures,
            entry.get(),
            entry->mName,
            defaultState,
            fileDefinition,
            "_mat"
        );

        materialList->AddPrimitive("&" + name);

        fileDefinition.AddMacro(MaterialIndexMacroName(entry->mName), std::to_string(index));

        ++index;
    }

    unsigned transparentIndex = 0;

    while (transparentIndex < materials.size() && sortOrderForMaterial(*materials[transparentIndex]) != TRANSPARENT_ORDER) {
        ++transparentIndex;
    }

    fileDefinition.AddMacro(fileDefinition.GetMacroName("MATERIAL_COUNT"), std::to_string(index));
    fileDefinition.AddMacro(fileDefinition.GetMacroName("TRANSPARENT_START"), std::to_string(transparentIndex + 1));

    // No revert list. The RDP is put back by a second display list; the GU is
    // told the new state instead, so there is nothing to undo.
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition(
        "const struct PspMaterial*", fileDefinition.GetUniqueName("material_list"), true, "_mat", std::move(materialList))));
}

void MaterialGenerator::GenerateDefinitions(const aiScene* scene, CFileDefinition& fileDefinition) {
    std::set<std::shared_ptr<TextureDefinition>> textures;
    std::set<std::shared_ptr<PaletteDefinition>> palettes;

    for (auto& entry : mSettings.mMaterials) {
        if (entry.second->mExcludeFromOutut) {
            continue;
        }

        for (int i = 0; i < 8; ++i) {
            std::shared_ptr<TextureDefinition> texture = entry.second->mState.tiles[i].texture;
            if (texture) {
                textures.insert(texture);

                if (texture->GetPalette()) {
                    palettes.insert(texture->GetPalette());
                }
            }
        }
    }

    std::vector<std::shared_ptr<Material>> materialsAsVector;

    for (auto& entry : mSettings.mMaterials) {
        if (entry.second->mExcludeFromOutut) {
            continue;
        }

        materialsAsVector.push_back(entry.second);
    }

    std::sort(materialsAsVector.begin(), materialsAsVector.end(), [&](const std::shared_ptr<Material>& a, const std::shared_ptr<Material>& b) -> bool {
        int aOrder = sortOrderForMaterial(*a);
        int bOrder = sortOrderForMaterial(*b);

        if (aOrder != bOrder) {
            return aOrder < bOrder;
        }

        return a->mSortOrder < b->mSortOrder;
    });

    if (mSettings.mTargetPsp) {
        GeneratePspDefinitions(fileDefinition, materialsAsVector);
        return;
    }

    for (auto& texture : textures) {
        auto textureDefinition = texture->GenerateDefinition(fileDefinition.GetUniqueName(texture->Name()), "_mat");
        fileDefinition.AddDefinition(std::move(textureDefinition));
    }

    for (auto& palette : palettes) {
        auto paletteDefinition = palette->GenerateDefinition(fileDefinition.GetUniqueName(palette->Name()), "_mat");
        fileDefinition.AddDefinition(std::move(paletteDefinition));
    }
    
    int index = 0;

    std::unique_ptr<StructureDataChunk> materialList(new StructureDataChunk());
    std::unique_ptr<StructureDataChunk> revertList(new StructureDataChunk());

    for (auto& entry : materialsAsVector) {
        std::string name = fileDefinition.GetUniqueName(entry->mName);

        DisplayList dl(name);
        if (entry->mName == mSettings.mDefaultMaterialName) {
            entry->Write(fileDefinition, MaterialState(), dl.GetDataChunk(), mSettings.mTargetCIBuffer);
        } else {
            entry->Write(fileDefinition, mSettings.mDefaultMaterialState, dl.GetDataChunk(), mSettings.mTargetCIBuffer);
        }
        std::unique_ptr<FileDefinition> material = dl.Generate("_mat");
        materialList->AddPrimitive(material->GetName());
        fileDefinition.AddDefinition(std::move(material));

        std::string revertName = fileDefinition.GetUniqueName(entry->mName + "_revert");
        DisplayList revertDL(revertName);
        generateMaterial(fileDefinition, entry->mState, mSettings.mDefaultMaterialState, revertDL.GetDataChunk(), mSettings.mTargetCIBuffer);
        std::unique_ptr<FileDefinition> materialRevert = revertDL.Generate("_mat");
        revertList->AddPrimitive(materialRevert->GetName());
        fileDefinition.AddDefinition(std::move(materialRevert));

        fileDefinition.AddMacro(MaterialIndexMacroName(entry->mName), std::to_string(index));

        ++index;
    }

    unsigned transparentIndex = 0;

    while (transparentIndex < materialsAsVector.size() && sortOrderForMaterial(*materialsAsVector[transparentIndex]) != TRANSPARENT_ORDER) {
        ++transparentIndex;
    }

    fileDefinition.AddMacro(fileDefinition.GetMacroName("MATERIAL_COUNT"), std::to_string(index));
    fileDefinition.AddMacro(fileDefinition.GetMacroName("TRANSPARENT_START"), std::to_string(transparentIndex + 1));

    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("Gfx*", fileDefinition.GetUniqueName("material_list"), true, "_mat", std::move(materialList))));
    fileDefinition.AddDefinition(std::unique_ptr<FileDefinition>(new DataFileDefinition("Gfx*", fileDefinition.GetUniqueName("material_revert_list"), true, "_mat", std::move(revertList))));
}

std::string MaterialGenerator::MaterialIndexMacroName(const std::string& materialName) {
    std::string result = materialName;
    std::transform(materialName.begin(), materialName.end(), result.begin(), ::toupper);
    makeCCompatible(result);
    return result + "_INDEX";
}
