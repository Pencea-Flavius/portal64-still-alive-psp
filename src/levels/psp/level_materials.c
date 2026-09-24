#include "level_materials.h"

#include "graphics/psp/psp_model_render.h"
#include "levels/material_state.h"

#include "codegen/assets/materials/static.h"

int levelMaterialCount() {
    return STATIC_MATERIAL_COUNT;
}

int levelMaterialTransparentStart() {
    return STATIC_TRANSPARENT_START;
}

const struct PspMaterial* levelMaterial(int index) {
    if (index < 0 || index >= STATIC_MATERIAL_COUNT) {
        return NULL;
    }

    return static_material_list[index];
}

const struct PspMaterial* levelMaterialDefault() {
    return static_material_list[DEFAULT_INDEX];
}

// Applying a material here is telling the GU the new state, with nothing to
// put back -- the N64's revert has no counterpart. The declaration is shared;
// only this is not.
void materialStateSet(struct MaterialState* state, short materialIndex, struct RenderState* renderState) {
    (void)renderState;

    if (state->materialIndex == materialIndex) {
        return;
    }

    state->materialIndex = materialIndex;

    const struct PspMaterial* material = levelMaterial(materialIndex);

    if (material) {
        pspMaterialBind(material);
    }
}
