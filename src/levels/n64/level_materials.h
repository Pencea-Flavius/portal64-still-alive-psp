#ifndef __LEVELS_N64_LEVEL_MATERIALS_H__
#define __LEVELS_N64_LEVEL_MATERIALS_H__

// The N64 half of the level's material list; see src/levels/psp/level_materials.h.
//
// A material here is a display list that sets RDP state, and reverting one is
// a second display list that puts it back.

#include <ultra64.h>

Gfx* levelMaterial(int index);
Gfx* levelMaterialDefault();
Gfx* levelMaterialRevert(int index);

#endif
