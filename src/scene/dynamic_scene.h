#ifndef __DYNAMIC_SCENE_H__
#define __DYNAMIC_SCENE_H__

#include "graphics/render_scene.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "scene/camera.h"

struct DynamicRenderDataList;

typedef int  (*DynamicCull)(void* data, struct FrustumCullingInformation* frustum);
typedef void (*DynamicRender)(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);
typedef void (*DynamicViewRender)(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#define MAX_DYNAMIC_SCENE_OBJECTS  64
#define MAX_VIEW_DEPENDENT_OBJECTS 24

#define DYNAMIC_SCENE_OBJECT_FLAGS_USED                 (1 << 0)
#define DYNAMIC_SCENE_OBJECT_SKIP_ROOT                  (1 << 1)

#define INVALID_DYNAMIC_OBJECT  -1

#define ROOM_FLAG_FROM_INDEX(flag) (1 << (flag))

struct DynamicSceneObject {
    void* data;
    struct Vector3* position;
    float scaledRadius;
    DynamicCull preciseCullingCallback;
    union {
        DynamicRender renderCallback;           // For objects
        DynamicViewRender viewRenderCallback;   // For viewDependentObjects
    };
    u16 flags;
    u64 roomFlags;
};

struct DynamicScene {
    struct DynamicSceneObject objects[MAX_DYNAMIC_SCENE_OBJECTS];
    struct DynamicSceneObject viewDependentObjects[MAX_VIEW_DEPENDENT_OBJECTS];
};

void dynamicSceneInit();

int dynamicSceneAdd(void* data, DynamicRender renderCallback, struct Vector3* position, float radius);
int dynamicSceneAddViewDependent(void* data, DynamicViewRender renderCallback, struct Vector3* position, float radius);
int dynamicSceneObjectCount();
int dynamicSceneViewDependentObjectCount();

void dynamicSceneRemove(int id);
void dynamicSceneSetFlags(int id, int flags);
void dynamicSceneClearFlags(int id, int flags);

void dynamicSceneSetRoomFlags(int id, u64 roomFlags);
void dynamicSceneSetPreciseCullingCallback(int id, DynamicCull callback);

void dynamicRenderListAddData(
    struct DynamicRenderDataList* list,
    ModelHandle model,
    RenderMatrices transform,
    short materialIndex,
    struct Vector3* position,
    RenderMatrices armature
);

void dynamicRenderListAddDataTouchingPortal(
    struct DynamicRenderDataList* list,
    ModelHandle model,
    RenderMatrices transform,
    short materialIndex,
    struct Vector3* position,
    RenderMatrices armature,
    int rigidBodyFlags
);

#endif
