#include "level_materials.h"

#include "levels/material_state.h"

#include "codegen/assets/materials/static.h"

int levelMaterialCount() {
    return STATIC_MATERIAL_COUNT;
}

int levelMaterialTransparentStart() {
    return STATIC_TRANSPARENT_START;
}

Gfx* levelMaterial(int index) {
    if (index < 0 || index >= STATIC_MATERIAL_COUNT) {
        return NULL;
    }

    return static_material_list[index];
}

Gfx* levelMaterialDefault() {
    return static_material_list[DEFAULT_INDEX];
}

Gfx* levelMaterialRevert(int index) {
    if (index < 0 || index >= STATIC_MATERIAL_COUNT) {
        return NULL;
    }

    return static_material_revert_list[index];
}

// Applying a material here is the material's display list, preceded by the
// revert of the one it replaces. The declaration is shared; only this is not.
void materialStateSet(struct MaterialState* state, short materialIndex, struct RenderState* renderState) {
    if (state->materialIndex != materialIndex) {
        if (state->materialIndex != -1) {
            gSPDisplayList(renderState->dl++, levelMaterialRevert(state->materialIndex));
        }

        state->materialIndex = materialIndex;

        gSPDisplayList(renderState->dl++, levelMaterial(materialIndex));
    }
}
