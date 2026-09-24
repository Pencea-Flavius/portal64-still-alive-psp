#ifndef __SK64_N64_SKELETOOL_RENDER_H__
#define __SK64_N64_SKELETOOL_RENDER_H__

// The N64 half of armature drawing (src/sk64/psp/...).

#include <ultra64.h>

#include "graphics/renderstate.h"
#include "sk64/skeletool_armature.h"

Gfx* skBuildAttachments(struct SKArmature* object, Gfx** attachments, struct RenderState* renderState);
void skRenderObject(struct SKArmature* object, Gfx** attachements, struct RenderState* intoState);
void skCalculateTransforms(struct SKArmature* object, Mtx* into);

// This frame's bone transforms, in frame memory. NULL when the frame is full.
RenderMatrices skArmatureBuildTransforms(struct SKArmature* armature, struct RenderState* renderState);

#endif
