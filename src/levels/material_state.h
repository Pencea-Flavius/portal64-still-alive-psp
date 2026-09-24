#ifndef __MATERIAL_STATE_H__
#define __MATERIAL_STATE_H__

#include "../graphics/renderstate.h"

struct MaterialState {
    short materialIndex;
};

void materialStateInit(struct MaterialState* state, short initialMaterial);

// The signature is the same on both machines, but applying a material is not:
// the N64 submits the material's display list and the previous one's revert,
// the GU is simply told the new state. So the body lives beside the level's
// material list, in levels/{n64,psp}/level_materials.c.
void materialStateSet(struct MaterialState* state, short materialIndex, struct RenderState* renderState);

#endif