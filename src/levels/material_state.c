#include "material_state.h"

void materialStateInit(struct MaterialState* state, short initialMaterial) {
    state->materialIndex = initialMaterial;
}
