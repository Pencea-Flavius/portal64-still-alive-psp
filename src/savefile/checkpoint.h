#ifndef __SAVEFILE_CHECKPOINT_H__
#define __SAVEFILE_CHECKPOINT_H__

// Only the tag is needed here. Including scene/scene.h pulled the whole scene
// graph into every menu that saves a setting.
struct Scene;

#define MAX_CHECKPOINT_SIZE 2048

typedef void* Checkpoint;

int checkpointExists();
void checkpointClear();

int checkpointSave(struct Scene* scene, int slotIndex);
void checkpointQueueLoad(int slotIndex);
void checkpointLoadCurrent(struct Scene* scene);

#endif
