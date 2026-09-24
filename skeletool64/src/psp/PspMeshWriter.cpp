#include "PspMeshWriter.h"
#include "../StringUtils.h"

#include <iostream>
#include <map>
#include <iomanip>
#include <sstream>

#include "../definitions/DataChunk.h"
#include "../materials/Material.h"
#include "../materials/TextureDefinition.h"
#include "PspMaterialWriter.h"

// From CFileDefinition.cpp, so PSP geometry matches the N64's exactly.
ErrorResult convertToShort(float value, short& output);
int convertNormalizedRange(float value);
unsigned convertByteRange(float value);

namespace {

// Mirrors PSP_VERTEX_FORMAT_* in src/graphics/psp/psp_vertex.h.
const char* vertexFormatMacro(VertexType vertexType) {
    return vertexType == VertexType::PosUVColor
        ? "PSP_VERTEX_FORMAT_COLOR"
        : "PSP_VERTEX_FORMAT_NORMAL";
}

// The GE needs vertices and texels 16 byte aligned; misaligned textures
// sample at an offset.
#define PSP_DATA_ALIGNMENT " __attribute__((aligned(16)))"

const char* vertexStructName(VertexType vertexType) {
    return vertexType == VertexType::PosUVColor
        ? "struct PspVertexColor" PSP_DATA_ALIGNMENT
        : "struct PspVertexNormal" PSP_DATA_ALIGNMENT;
}

// The RDP tile shift: 1-10 divides by 2^shift, 11-15 multiplies by
// 2^(16 - shift) (the projectile's glow uses 1).
float pspTileShiftScale(int shift) {
    if (shift >= 1 && shift <= 10) {
        return 1.0f / (float)(1 << shift);
    }

    if (shift >= 11 && shift <= 15) {
        return (float)(1 << (16 - shift));
    }

    return 1.0f;
}

bool appendPosition(
    const std::shared_ptr<ExtendedMesh>& mesh,
    unsigned index,
    float fixedPointScale,
    StructureDataChunk& vertex
) {
    aiVector3D pos = mesh->mMesh->mVertices[index];

    if (mesh->mPointInverseTransform.size()) {
        pos = mesh->mPointInverseTransform[index] * pos;
    }

    pos = pos * fixedPointScale;

    short converted;

    for (int axis = 0; axis < 3; ++axis) {
        if (convertToShort(pos[axis], converted).HasError()) {
            std::cerr << "Vertex coordinate does not fit into a short" << std::endl;
            return false;
        }

        vertex.AddPrimitive(converted);
    }

    return true;
}

void appendUV(
    const std::shared_ptr<ExtendedMesh>& mesh,
    unsigned index,
    float uScale,
    float vScale,
    StructureDataChunk& vertex
) {
    // Normalise against the texture actually bound (mirror doubled, padded to a
    // power of two).
    float u = 0.0f;
    float v = 0.0f;

    if (mesh->mMesh->mTextureCoords[0] != nullptr) {
        aiVector3D uv = mesh->mMesh->mTextureCoords[0][index];

        u = uv.x * uScale;
        // V is flipped: the exporter measures it upwards.
        v = (1.0f - uv.y) * vScale;
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6) << u << "f";
    vertex.AddPrimitive(stream.str());

    stream.str("");
    stream << std::fixed << std::setprecision(6) << v << "f";
    vertex.AddPrimitive(stream.str());
}

void appendNormal(
    const std::shared_ptr<ExtendedMesh>& mesh,
    unsigned index,
    const aiQuaternion& rotate,
    unsigned defaultAlpha,
    StructureDataChunk& vertex
) {
    aiVector3D normal;

    if (mesh->mMesh->HasNormals()) {
        normal = mesh->mMesh->mNormals[index];
    }

    if (mesh->mPointInverseTransform.size()) {
        normal = mesh->mNormalInverseTransform[index] * normal;
        normal.NormalizeSafe();
    } else {
        normal = rotate.Rotate(normal);
    }

    vertex.AddPrimitive(convertNormalizedRange(normal.x));
    vertex.AddPrimitive(convertNormalizedRange(normal.y));
    vertex.AddPrimitive(convertNormalizedRange(normal.z));
    // The byte after the normal is the alpha, as in the N64's Vtx_tn; the GU
    // skips it and fading materials read it back.
    unsigned alpha = defaultAlpha;

    if (mesh->mMesh->mColors[0] != nullptr) {
        float a = mesh->mMesh->mColors[1] != nullptr
            ? mesh->mMesh->mColors[1][index].r
            : mesh->mMesh->mColors[0][index].a;
        alpha = convertByteRange(a);
    }

    vertex.AddPrimitive(alpha);
}

void appendColor(
    const std::shared_ptr<ExtendedMesh>& mesh,
    unsigned index,
    const PixelRGBAu8& defaultVertexColor,
    StructureDataChunk& vertex
) {
    unsigned r = defaultVertexColor.r;
    unsigned g = defaultVertexColor.g;
    unsigned b = defaultVertexColor.b;
    unsigned a = defaultVertexColor.a;

    if (mesh->mMesh->mColors[0] != nullptr) {
        aiColor4D color = mesh->mMesh->mColors[0][index];

        if (mesh->mMesh->mColors[1] != nullptr) {
            color.a = mesh->mMesh->mColors[1][index].r;
        }

        r = convertByteRange(color.r);
        g = convertByteRange(color.g);
        b = convertByteRange(color.b);
        a = convertByteRange(color.a);
    }

    // GU_COLOR_8888 is ABGR in memory, not RGBA.
    std::ostringstream packed;
    packed << "0x" << std::hex << std::uppercase
           << ((a << 24) | (b << 16) | (g << 8) | r);

    vertex.AddPrimitive(packed.str());
}

// RDP render mode bits.
}

std::string generatePspMesh(
    const aiScene* scene,
    CFileDefinition& fileDefinition,
    std::vector<RenderChunk>& renderChunks,
    DisplayListSettings& settings,
    const std::string& fileSuffix
) {
    std::unique_ptr<StructureDataChunk> partsChunk(new StructureDataChunk());
    PspEmittedTextures emittedTextures;
    std::map<Material*, std::string> emittedMaterials;
    // Bone to parent for every bone used and its ancestors: poses are
    // parent-relative.
    std::map<int, int> boneParents;
    unsigned partCount = 0;

    for (auto chunk = renderChunks.begin(); chunk != renderChunks.end(); ++chunk) {
        if (!chunk->mMesh) {
            continue;
        }

        const std::vector<aiFace*>& faces = chunk->GetFaces();

        if (faces.empty()) {
            continue;
        }

        // Vertices are in bone space (appendPosition() applies
        // mPointInverseTransform), like the N64's RCPState::TraverseToBone().
        int boneIndex = Bone::GetBoneIndex(chunk->mBonePair.first);

        // A chunk across a joint spans two bones; the N64 loads each vertex under
        // its own bone (flushVertices()), so the part also gets per vertex bones.
        bool spansBones = chunk->mBonePair.first != chunk->mBonePair.second;
        int secondBoneIndex = spansBones ? Bone::GetBoneIndex(chunk->mBonePair.second) : -1;

        for (Bone* bone = chunk->mBonePair.first; bone; bone = bone->GetParent()) {
            boneParents[bone->GetIndex()] = Bone::GetBoneIndex(bone->GetParent());
        }

        for (Bone* bone = spansBones ? chunk->mBonePair.second : nullptr; bone; bone = bone->GetParent()) {
            boneParents[bone->GetIndex()] = Bone::GetBoneIndex(bone->GetParent());
        }

        // Every vertex's bone goes in the table, however many a chunk holds.
        if (spansBones) {
            for (auto face : faces) {
                for (unsigned i = 0; i < face->mNumIndices; ++i) {
                    unsigned vertex = face->mIndices[i];
                    Bone* own = vertex < chunk->mMesh->mVertexBones.size() ? chunk->mMesh->mVertexBones[vertex] : nullptr;

                    for (Bone* bone = own; bone; bone = bone->GetParent()) {
                        boneParents[bone->GetIndex()] = Bone::GetBoneIndex(bone->GetParent());
                    }
                }
            }
        }

        VertexType vertexType = Material::GetVertexType(chunk->mMaterial);

        // The original image's share of the bound texture.
        float uScale = 1.0f;
        float vScale = 1.0f;

        if (chunk->mMaterial) {
            const TileState& tile = pspTexelTile(chunk->mMaterial->mState);
            const std::shared_ptr<TextureDefinition>& texture = tile.texture;

            if (texture) {
                int boundWidth = TextureDefinition::PspPaddedSize(
                    texture->Width() * (tile.sCoord.mirror ? 2 : 1));
                int boundHeight = TextureDefinition::PspPaddedSize(
                    texture->Height() * (tile.tCoord.mirror ? 2 : 1));

                uScale = (float)texture->Width() / (float)boundWidth * pspTileShiftScale(tile.sCoord.shift);
                vScale = (float)texture->Height() / (float)boundHeight * pspTileShiftScale(tile.tCoord.shift);
            }
        }

        // Only the chunk's vertices, with indices renumbered.
        std::map<unsigned, unsigned> indexRemap;
        std::vector<unsigned> orderedVertices;

        for (auto face : faces) {
            for (unsigned i = 0; i < face->mNumIndices; ++i) {
                unsigned original = face->mIndices[i];

                if (indexRemap.find(original) == indexRemap.end()) {
                    indexRemap[original] = orderedVertices.size();
                    orderedVertices.push_back(original);
                }
            }
        }

        // A vertex colour replaces the material's on the GE, so materials that
        // ignore shade get their constant colour here (the glass's 156 alpha).
        const PspColorSource fragment = chunk->mMaterial
            ? pspAnalyzeCombine(chunk->mMaterial->mState).fragment
            : PspColorSource::Shade;
        const bool usesShade = fragment == PspColorSource::Shade;

        std::unique_ptr<StructureDataChunk> vertices(new StructureDataChunk());

        for (unsigned original : orderedVertices) {
            std::unique_ptr<StructureDataChunk> vertex(new StructureDataChunk());

            // The GU requires this order: texture, colour, normal, position.
            appendUV(chunk->mMesh, original, uScale, vScale, *vertex);

            if (vertexType == VertexType::PosUVColor && !usesShade) {
                vertex->AddPrimitive(pspColorOf(chunk->mMaterial->mState, fragment));
            } else if (vertexType == VertexType::PosUVColor) {
                appendColor(chunk->mMesh, original, chunk->mMaterial->mDefaultVertexColor, *vertex);
            } else {
                appendNormal(chunk->mMesh, original, settings.mRotateModel, chunk->mMaterial->mDefaultVertexColor.a, *vertex);
            }

            if (!appendPosition(chunk->mMesh, original, settings.mFixedPointScale, *vertex)) {
                return "";
            }

            // Trailing padding, so the struct matches what the GU reads.
            vertex->AddPrimitive(0);

            vertices->Add(std::move(vertex));
        }

        std::unique_ptr<StructureDataChunk> indices(new StructureDataChunk());
        unsigned indexCount = 0;

        for (auto face : faces) {
            if (face->mNumIndices != 3) {
                std::cerr << "Expected triangles, found a face with "
                          << face->mNumIndices << " indices" << std::endl;
                return "";
            }

            for (unsigned i = 0; i < 3; ++i) {
                indices->AddPrimitive(indexRemap[face->mIndices[i]]);
                ++indexCount;
            }
        }

        // Named like the N64's Vtx array, since game code finds them by name (the
        // clock scrolling a digit's UVs).
        std::string bufferName = chunk->mMesh->mMesh->mName.length
            ? std::string(chunk->mMesh->mMesh->mName.C_Str())
            : std::string("_mesh");

        switch (vertexType) {
            case VertexType::PosUVColor:
                bufferName += "_color";
                break;
            case VertexType::PosUVNormal:
                bufferName += "_normal";
                break;
            default:
                break;
        }

        std::string vertexName = fileDefinition.AddDataDefinition(
            bufferName, vertexStructName(vertexType), true, fileSuffix, std::move(vertices));
        std::string indexName = fileDefinition.AddDataDefinition(
            bufferName + "_indices", "unsigned short" PSP_DATA_ALIGNMENT, true, fileSuffix, std::move(indices));

        std::string materialName = ExtendedMesh::GetMaterialName(
            scene->mMaterials[chunk->mMesh->mMesh->mMaterialIndex], settings.mForceMaterialName);

        // A material from a -m file is referenced from that file's list rather
        // than emitted again.
        std::string materialReference;

        if (settings.mPspSharedMaterialNames.count(materialName)) {
            fileDefinition.AddHeader(
                "\"codegen/assets/materials/" + settings.mPspSharedMaterials + ".h\"");
            materialReference = settings.mPspSharedMaterials + "_" + materialName + "_material";
            // Same sanitizer as the list ("plastic/plasticwall001a").
            makeCCompatible(materialReference);
        } else {
            materialReference = usePspMaterial(
                emittedMaterials, emittedTextures, chunk->mMaterial, materialName,
                settings.mDefaultMaterialState, fileDefinition, fileSuffix);
        }

        std::unique_ptr<StructureDataChunk> part(new StructureDataChunk());
        part->AddPrimitive(vertexName);
        part->AddPrimitive(indexName);
        part->AddPrimitive(indexCount);
        part->AddPrimitive((unsigned)orderedVertices.size());
        part->AddPrimitive(std::string(vertexFormatMacro(vertexType)));
        if (settings.mPspPartMaterialFromScene ||
            (settings.mPspDefaultMaterialFromScene && materialName == settings.mDefaultMaterialName)) {
            part->AddPrimitive(std::string("NULL"));
        } else {
            part->AddPrimitive("&" + materialReference);
        }
        part->AddPrimitive(boneIndex);
        part->AddPrimitive(secondBoneIndex);

        if (spansBones) {
            std::unique_ptr<StructureDataChunk> sides(new StructureDataChunk());

            // Each vertex's bone, 255 for none.
            for (unsigned original : orderedVertices) {
                Bone* own = original < chunk->mMesh->mVertexBones.size() ? chunk->mMesh->mVertexBones[original] : nullptr;
                int index = Bone::GetBoneIndex(own);
                sides->AddPrimitive(index < 0 || index > 254 ? 255 : index);
            }

            part->AddPrimitive(fileDefinition.AddDataDefinition(
                bufferName + "_bones", "unsigned char", true, fileSuffix, std::move(sides)));
        } else {
            part->AddPrimitive(std::string("0"));
        }

        partsChunk->Add(std::move(part));
        ++partCount;
    }

    std::string partsName = fileDefinition.AddDataDefinition(
        "parts", "struct PspModelPart", true, fileSuffix, std::move(partsChunk));

    // The bone table, indexed like the pose; only for models with bones.
    std::string boneParentName;
    int boneCount = 0;

    if (!boneParents.empty()) {
        boneCount = boneParents.rbegin()->first + 1;

        std::unique_ptr<StructureDataChunk> parents(new StructureDataChunk());

        for (int i = 0; i < boneCount; ++i) {
            auto entry = boneParents.find(i);
            int parent = entry == boneParents.end() ? -1 : entry->second;

            parents->AddPrimitive(parent < 0
                ? std::string("PSP_NO_BONE_PARENT")
                : std::to_string(parent));
        }

        boneParentName = fileDefinition.AddDataDefinition(
            "bone_parents", "unsigned short", true, fileSuffix, std::move(parents));
    }

    std::unique_ptr<StructureDataChunk> model(new StructureDataChunk());
    model->AddPrimitive(partsName);
    model->AddPrimitive(partCount);
    model->AddPrimitive(boneParentName.empty() ? std::string("0") : boneParentName);
    model->AddPrimitive(boneCount);

    fileDefinition.AddHeader("\"graphics/psp/psp_model.h\"");

    std::string modelName = fileDefinition.AddDataDefinition(
        "model", "struct PspModel", false, fileSuffix, std::move(model));

    // The N64 declares `Gfx <name>_model_gfx[]`; alias the struct's address
    // under that name so shared code can name the model on both machines.
    fileDefinition.AddMacro(modelName + "_gfx", "(&" + modelName + ")");

    return modelName;
}

std::string generatePspVertexBuffer(
    CFileDefinition& fileDefinition,
    const std::shared_ptr<ExtendedMesh>& mesh,
    Material* material,
    const DisplayListSettings& settings,
    const std::string& fileSuffix
) {
    VertexType vertexType = Material::GetVertexType(material);

    float uScale = 1.0f;
    float vScale = 1.0f;

    if (material) {
        const TileState& tile = pspTexelTile(material->mState);

        if (tile.texture) {
            int boundWidth = TextureDefinition::PspPaddedSize(
                tile.texture->Width() * (tile.sCoord.mirror ? 2 : 1));
            int boundHeight = TextureDefinition::PspPaddedSize(
                tile.texture->Height() * (tile.tCoord.mirror ? 2 : 1));

            uScale = (float)tile.texture->Width() / (float)boundWidth * pspTileShiftScale(tile.sCoord.shift);
            vScale = (float)tile.texture->Height() / (float)boundHeight * pspTileShiftScale(tile.tCoord.shift);
        }
    }

    PixelRGBAu8 defaultVertexColor = material
        ? material->mDefaultVertexColor
        : PixelRGBAu8();

    std::unique_ptr<StructureDataChunk> vertices(new StructureDataChunk());

    // The whole mesh in order; no chunk to renumber against.
    for (unsigned index = 0; index < mesh->mMesh->mNumVertices; ++index) {
        std::unique_ptr<StructureDataChunk> vertex(new StructureDataChunk());

        // The GU requires this order: texture, colour, normal, position.
        appendUV(mesh, index, uScale, vScale, *vertex);

        if (vertexType == VertexType::PosUVColor) {
            appendColor(mesh, index, defaultVertexColor, *vertex);
        } else {
            appendNormal(mesh, index, settings.mRotateModel, defaultVertexColor.a, *vertex);
        }

        if (!appendPosition(mesh, index, settings.mFixedPointScale, *vertex)) {
            return "";
        }

        // Trailing padding, so the struct matches what the GU reads.
        vertex->AddPrimitive(0);

        vertices->Add(std::move(vertex));
    }

    // Named like the N64's Vtx array.
    std::string bufferName = mesh->mMesh->mName.length
        ? std::string(mesh->mMesh->mName.C_Str())
        : std::string("_mesh");

    switch (vertexType) {
        case VertexType::PosUVColor:
            bufferName += "_color";
            break;
        case VertexType::PosUVNormal:
            bufferName += "_normal";
            break;
        default:
            break;
    }

    return fileDefinition.AddDataDefinition(
        bufferName, vertexStructName(vertexType), true, fileSuffix, std::move(vertices));
}
