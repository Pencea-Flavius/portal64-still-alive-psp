#ifndef __PSP_MODEL_RENDER_H__
#define __PSP_MODEL_RENDER_H__

#include "psp_model.h"
#include "render_state.h"

// Draws every part of a model; matrices and materials are the caller's.
//
// Sets the GU up for a material: texture and mips, texture function, colour,
// depth, lighting and blending. 2D sprites use it too.
void pspMaterialBind(const struct PspMaterial* material);

// Draws a model under a pose (one parent-relative matrix per bone, from
// skArmatureBuildTransforms() or sceneAnimatorBuildTransforms()). Parts
// with no bone are drawn in model space.
void pspModelDrawSkinned(const struct PspModel* model, const void* boneMatrices);

// The same with only level 0 bound, for 2D: with texel UVs in
// GU_TRANSFORM_2D the GE picks a random mip level and reads past it.
void pspMaterialBindSprite(const struct PspMaterial* material);

void pspModelDraw(const struct PspModel* model);

// A one part model in the frame's scratch, for geometry built this frame
// (laser beam, doorway cover, particles). NULL when the scratch is full.
struct PspModel* pspModelBuild(
    struct RenderState* renderState,
    const void* vertices,
    unsigned short vertexCount,
    const unsigned short* indices,
    unsigned short indexCount,
    unsigned int vertexFormat,
    const struct PspMaterial* material
);

// The N64's gSPLookAt directions for reflection mapping, carried by two
// lights that are never enabled.
#define PSP_LOOKAT_LIGHT_S  2
#define PSP_LOOKAT_LIGHT_T  3

struct Vector3;
void pspSetLookAt(const struct Vector3* s, const struct Vector3* t);

// The most a decal is pulled towards the camera, in depth units; less far
// away (see psp_model_render.c).
#define PSP_DECAL_DEPTH_OFFSET 16

// Pulls everything drawn after towards the camera by this many depth units,
// on top of decals, until set back to 0.
void pspModelSetDepthBias(int bias);

// Converts world distances into depth units at a position, since a unit is
// millimetres close up and metres far away. Set once a stage; metres.
void pspDepthSetView(float slice, float nearPlane, float farPlane, const struct Vector3* eye, const struct Vector3* forward);

// How many depth units `metres` spans at `at`, clamped to [minUnits, maxUnits].
int pspDepthUnitsFor(const struct Vector3* at, float metres, int minUnits, int maxUnits);

// Fog start and end along the view, in scene units; set once a stage.
void pspSetFogRange(float nearDistance, float farDistance);

// Opens the depth window up to the near end of the stage set last with
// pspDepthSetWindow() (called by renderViewportApply()).
void pspDepthSetWindow(int nearZ, int farZ);
// The whole buffer again, for the HUD and menus at depth 0.
void pspDepthResetWindow();
void pspWidenDepthWindow();

// Call after setting GU_PROJECTION or GU_VIEW; the clip test caches their
// product.
void pspModelViewChanged();
// The exit portal plane (normal, d; inside positive) whose far side a
// portal view drops per triangle, or NULL.
void pspSetStageCullPlane(const float* worldPlane);
// Nonzero while drawing a portal view: small parts are skipped sooner.
void pspSetPortalView(int throughPortal);

// Nonzero skips the edge test for what is drawn until it is set back to 0.
void pspModelSetNoClip(int noClip);

// Copies textures into free VRAM while they fit. Once, at boot.
void pspMaterialsToVram(const struct PspMaterial* const* materials, int count);

// Call after setting material state directly: the next bind sends it all.
void pspMaterialForgetState();

// At the start of each frame: resend the texture, drop last frame's skinning.
void pspModelStartFrame();

// Around a view's sorted scene: skips rebinding the same material.
void pspModelBeginBatch();
void pspModelEndBatch();

// Diagnostics: autoLod off pins level 0; mipmaps off binds only the top.
void pspModelSetLodOverride(int autoLod, int mipmaps, float lodBias);

#endif
