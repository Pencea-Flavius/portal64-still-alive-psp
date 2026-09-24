
#include "skeletool_armature.h"
#include "skeletool_defs.h"
#include "system/cartridge.h"
#include "util/memory.h"

void skArmatureInit(struct SKArmature* object, struct SKArmatureDefinition* definition) {
    unsigned transformSize = sizeof(struct Transform) * definition->numberOfBones;
    skArmatureInitWithPose(object, definition, malloc(transformSize));
}

void skArmatureInitWithPose(struct SKArmature* object, struct SKArmatureDefinition* definition, struct Transform* usePose) {
    object->displayList = definition->displayList;
    object->numberOfBones = definition->numberOfBones;
    object->numberOfAttachments = definition->numberOfAttachments;
    object->pose = usePose;
    object->boneParentIndex = definition->boneParentIndex;

    unsigned transformSize = sizeof(struct Transform) * definition->numberOfBones;
    
    if (definition->pose) {
        if (romIsInMemory(definition->pose)) {
            memCopy(object->pose, definition->pose, transformSize);
        } else {
            romCopy((void*)definition->pose, (void*)object->pose, transformSize);
        }
    }
}

void skArmatureGetCenter(struct SKArmature* object, struct Vector3* out) {
    struct Vector3 minPoint = object->pose[0].position;
    struct Vector3 maxPoint = minPoint;

    for (int i = 1; i < object->numberOfBones; ++i) {
        vector3Min(&minPoint, &object->pose[i].position, &minPoint);
        vector3Max(&maxPoint, &object->pose[i].position, &maxPoint);
    }

    vector3Add(&minPoint, &maxPoint, out);
    vector3Scale(out, out, 0.5f);
}

void skCleanupObject(struct SKArmature* object) {
    free(object->pose);
    object->pose = 0;
    object->numberOfBones = 0;
}

void skCalculateBonePosition(struct SKArmature* object, unsigned short boneIndex, struct Vector3* bonePosition, struct Vector3* out) {
    if (!object->boneParentIndex) {
        return;
    }
    *out = *bonePosition;

    while (boneIndex < object->numberOfBones) {
        transformPoint(&object->pose[boneIndex], out, out);
        boneIndex = object->boneParentIndex[boneIndex];
    }
}

void skCalculateBoneRotation(struct SKArmature* object, unsigned short boneIndex, struct Quaternion* out) {
    if (!object->boneParentIndex) {
        return;
    }

    quatIdent(out);

    while (boneIndex < object->numberOfBones) {
        struct Quaternion next;
        quatMultiply(&object->pose[boneIndex].rotation, out, &next);
        *out = next;
        boneIndex = object->boneParentIndex[boneIndex];
    }
}