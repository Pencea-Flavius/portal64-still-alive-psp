#ifndef _DISPLAY_LIST_SETTINGS_H
#define _DISPLAY_LIST_SETTINGS_H

#include <string>
#include <map>
#include <set>
#include <assimp/scene.h>
#include <memory>
#include "./materials/Material.h"
#include "./materials/MaterialState.h"

#define DEFAULT_MAX_OPTIMIZATION_ITERATIONS 1000

struct DisplayListSettings {
    DisplayListSettings();
    std::string mPrefix;
    int mVertexCacheSize;
    bool mHasTri2;
    float mFixedPointScale;
    float mModelScale;
    int mMaxMatrixDepth;
    int mMaxOptimizationIterations;
    bool mCanPopMultipleMatrices;
    float mTicksPerSecond;
    std::map<std::string, std::shared_ptr<Material>> mMaterials;
    std::string mDefaultMaterialName;
    std::string mForceMaterialName;
    std::string mForcePalette;
    MaterialState mDefaultMaterialState;
    aiQuaternion mRotateModel;
    bool mExportAnimation;
    bool mExportGeometry;
    bool mIncludeCulling;
    bool mBonesAsVertexGroups;
    // Emit PSP geometry instead of an F3DEX display list.
    bool mTargetPsp;
    // Materials already emitted as a list under this name are referenced as
    // <name>_<material> instead of copied.
    std::string mPspSharedMaterials;
    // The names that list defines.
    std::set<std::string> mPspSharedMaterialNames;
    // Level parts in the default material carry none, so the game binds the
    // static content's materialIndex (how signals swap indicator lights).
    bool mPspPartMaterialFromScene;
    // Only parts in the default material take the scene's (a decor object's,
    // as for fizzling objects).
    bool mPspDefaultMaterialFromScene;
    bool mTargetCIBuffer;

    aiVector3D mSortDirection;

    aiMatrix4x4 CreateGlobalTransform() const;
    aiMatrix4x4 CreateCollisionTransform() const;

    bool NeedsTangents() const;
};

#endif