#ifndef __LEVELS_PSP_LEVEL_MATERIALS_H__
#define __LEVELS_PSP_LEVEL_MATERIALS_H__

// The PSP half of the level's material list; see src/levels/n64/level_materials.h.
// Materials are PspMaterial and are never reverted, so there is no
// levelMaterialRevert().

#include "graphics/psp/psp_model.h"

const struct PspMaterial* levelMaterial(int index);
const struct PspMaterial* levelMaterialDefault();

#endif
