#ifndef __SK64_PSP_SKELETOOL_RENDER_H__
#define __SK64_PSP_SKELETOOL_RENDER_H__

// The PSP half of the armature's drawing; see src/sk64/n64/skeletool_render.h.
// No skRenderObject()/skBuildAttachments(): models go through the render
// scene, and attachments are drawn under skAttachmentTransform().

#include "graphics/renderstate.h"

// Forward declared: skeletool_armature.h includes this header.
struct SKArmature;

// The armature's bone transforms in frame memory, ready to draw. NULL when
// the frame is full.
RenderMatrices skArmatureBuildTransforms(struct SKArmature* armature, struct RenderState* renderState);

// The pose as parent-relative matrices into a caller-owned block (the scene
// animator fills one from several armatures).
void skCalculateTransforms(struct SKArmature* object, RenderMatrix* into);

// The transform of something attached to a bone (<MODEL>_ATTACHMENT_<NAME>_BONE):
// the object's matrix, then the bone chain. The N64 branches from inside the
// model's display list instead. NULL when the frame is full.
RenderMatrices skAttachmentTransform(struct SKArmature* armature, int boneIndex, RenderMatrices objectMatrix, RenderMatrices bones, struct RenderState* renderState);

#endif
