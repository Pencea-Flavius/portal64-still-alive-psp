#ifndef _PSP_MESH_WRITER_H
#define _PSP_MESH_WRITER_H

#include <string>
#include <vector>

#include <assimp/scene.h>

#include "../CFileDefinition.h"
#include "../DisplayListSettings.h"
#include "../RenderChunk.h"

// The PSP counterpart to generateMesh(). Everything upstream of this -- scene
// loading, bones, UV projection, chunk ordering -- is shared, because none of
// it is specific to the RCP.
std::string generatePspMesh(
    const aiScene* scene,
    CFileDefinition& fileDefinition,
    std::vector<RenderChunk>& renderChunks,
    DisplayListSettings& settings,
    const std::string& fileSuffix
);

// The PSP counterpart to CFileDefinition::GetVertexBuffer(): a whole mesh's
// vertices on their own, with no parts and no indices around them. The level
// exporter asks for these for a portal surface, whose vertices the game reads
// and re-cuts at runtime rather than handing straight to the hardware.
std::string generatePspVertexBuffer(
    CFileDefinition& fileDefinition,
    const std::shared_ptr<ExtendedMesh>& mesh,
    Material* material,
    const DisplayListSettings& settings,
    const std::string& fileSuffix
);

#endif
