#ifndef __EFFECTS_PSP_PARTICLE_EFFECT_RENDER_H__
#define __EFFECTS_PSP_PARTICLE_EFFECT_RENDER_H__

// The PSP half of a particle effect's drawing; see src/effects/n64/particle_effect_render.h.
//
// Both are callbacks the dynamic scene holds, which is why they take void*.

#include "graphics/render_scene.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "scene/dynamic_render_list.h"

void particleEffectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState);
void particleEffectRenderBillboarded(void* data, struct RenderScene* renderScene, struct Transform* fromView);

#endif
