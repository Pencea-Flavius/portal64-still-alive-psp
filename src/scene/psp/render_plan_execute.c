#include "scene/render_plan.h"

#include "graphics.h"

#include "graphics/psp/psp_render.h"
#include "levels/static_render.h"
#include "math/mathf.h"
#include "physics/collision_scene.h"
#include "scene/dynamic_scene.h"
#include "scene/portal_render.h"
#include "scene/scene.h"
#include "system/display.h"
#include "system/psp/psp_profile.h"
#include "savefile/savefile.h"

#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_vertex.h"

#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include "codegen/assets/models/portal/portal_blue.h"
#include "codegen/assets/models/portal/portal_blue_face.h"
#include "codegen/assets/models/portal/portal_orange.h"
#include "codegen/assets/models/portal/portal_orange_face.h"

// The rim and filled oval lie in the wall's plane, and GEQUAL gives ties to
// the wall drawn after them, so pull them forward by a distance turned into
// depth units (a fixed count is too much far away).
#define PORTAL_RIM_OFFSET       0.01f
#define PORTAL_RIM_MIN_BIAS     2
#define PORTAL_RIM_MAX_BIAS     32

static int portalRimDepthBias(struct Portal* portal) {
    return pspDepthUnitsFor(&portal->rigidBody.transform.position, PORTAL_RIM_OFFSET, PORTAL_RIM_MIN_BIAS, PORTAL_RIM_MAX_BIAS);
}

// How far a portal's face is pulled in front of a wall's decals, on top of
// their own pull.
#define PORTAL_FACE_DECAL_LEAD      0.02f
#define PORTAL_FACE_DECAL_MIN_BIAS  8
#define PORTAL_FACE_DECAL_MAX_BIAS  64

static int portalFaceDecalBias(struct Portal* portal) {
    return PSP_DECAL_DEPTH_OFFSET + pspDepthUnitsFor(&portal->rigidBody.transform.position,
        PORTAL_FACE_DECAL_LEAD, PORTAL_FACE_DECAL_MIN_BIAS, PORTAL_FACE_DECAL_MAX_BIAS);
}

// The camera and depth slice pspDepthUnitsFor() measures against.
static void renderStageSetDepthView(struct RenderProps* stage) {
    const struct RenderViewportRect* viewport = stage->viewport;
    struct Vector3 forward;
    quatMultVector(&stage->camera.transform.rotation, &gForward, &forward);
    vector3Negate(&forward, &forward);

    pspDepthSetView(
        viewport ? (float)(viewport->maxZ - viewport->minZ) : 0.0f,
        stage->camera.nearPlane,
        stage->camera.farPlane,
        &stage->camera.transform.position,
        &forward
    );
}


#define MIN_FOG_DISTANCE 1.0f
#define MAX_FOG_DISTANCE 2.5f

// The PSP half; see src/scene/n64/render_plan_execute.c. Stages are drawn
// back to front, each clipped to its rectangle and then to its portal's oval
// through the depth buffer (no stencil; see renderStageMaskToPortal()).

// The N64's gSceneLights: grey ambient and grey diffuse pointing straight up
// (gdSPDefLights1(128,128,128, 128,128,128, 0,127,0)).
static void renderPlanApplyLighting() {
    // The direction the light comes from: (0, 127, 0) normalised.
    ScePspFVector3 direction = {0.0f, 1.0f, 0.0f};

    sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &direction);
    sceGuLightColor(0, GU_DIFFUSE, 0xFF808080);
    sceGuAmbient(0xFF808080);
    sceGuEnable(GU_LIGHT0);
}

static void renderStageApply(struct RenderProps* current) {
    renderViewportApply(current->viewport);

    pspRenderSetScissor(
        current->minX,
        current->minY,
        current->maxX - current->minX,
        current->maxY - current->minY
    );
}

// Fog between two distances; sceGuFog takes them as floats.
static void renderStageApplyFog(struct RenderProps* current) {
    // Distances along the view; the colour is set per material.
    (void)current;
    pspSetFogRange(MIN_FOG_DISTANCE * SCENE_SCALE, MAX_FOG_DISTANCE * SCENE_SCALE);
}

// The portal's opacity goes into the face's vertices, since a vertex colour
// replaces the material's on the GE. One face model per colour.
static void portalFaceSetOpacity(const struct PspModel* face, float opacity) {
    unsigned alpha = opacity <= 0.0f ? 0 : opacity >= 1.0f ? 255 : (unsigned)(opacity * 255.0f);

    for (unsigned short i = 0; i < face->partCount; ++i) {
        const struct PspModelPart* part = &face->parts[i];

        if (part->vertexFormat != PSP_VERTEX_FORMAT_COLOR || !part->vertexCount) {
            continue;
        }

        struct PspVertexColor* vertices = (struct PspVertexColor*)part->vertices;

        if ((vertices[0].color >> 24) == alpha) {
            continue;
        }

        for (unsigned short v = 0; v < part->vertexCount; ++v) {
            vertices[v].color = (vertices[v].color & 0x00FFFFFF) | (alpha << 24);
        }

        sceKernelDcacheWritebackRange(vertices, sizeof(struct PspVertexColor) * part->vertexCount);
    }
}

// As on the N64: for each visible portal, the face and rim around its view,
// or the filled oval. The face writes the portal's depth.
static void renderStageDrawPortals(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* current, struct RenderState* renderState) {
    int portalIndex = (current->portalRenderType & PORTAL_RENDER_TYPE_SECOND_CLOSER) ? 1 : 0;

    for (int i = 0; i < 2; ++i) {
        if (current->portalRenderType & PORTAL_RENDER_TYPE_VISIBLE(portalIndex)) {
            float portalTransform[4][4];
            struct Portal* portal = &scene->portals[portalIndex];
            portalDetermineTransform(portal, portalTransform);

            struct RenderProps* portalProps = current->nextProperites[portalIndex];
            // On a wall with decals the rim and cover need the face's lead too.
            int rimBias = (portal->flags & PortalFlagsZOffset) ? portalFaceDecalBias(portal) : portalRimDepthBias(portal);

            if (portalProps && (current->portalRenderType & PORTAL_RENDER_TYPE_ENABLED(portalIndex))) {
                RenderMatrices matrix = renderStateMatrixFromFloat(renderState, portalTransform);

                if (matrix) {
                    const struct PspModel* face;
                    const struct PspModel* rim;

                    if (portal->flags & PortalFlagsOddParity) {
                        face = &portal_portal_blue_face_model;
                        rim = &portal_portal_blue_model;
                    } else {
                        face = &portal_portal_orange_face_model;
                        rim = &portal_portal_orange_model;
                    }

                    portalFaceSetOpacity(face, portal->opacity);

                    sceGumMatrixMode(GU_MODEL);
                    sceGumPushMatrix();
                    sceGumMultMatrix((const ScePspFMatrix4*)matrix);

                    // Past the decals if any, else as far as the rim. Kept small:
                    // it writes depth over the whole oval.
                    int faceBias = (portal->flags & PortalFlagsZOffset) ? portalFaceDecalBias(portal) : portalRimDepthBias(portal);
                    pspModelSetDepthBias(faceBias);
                    pspModelDraw(face);

                    if (current->previousProperties == NULL && portalIndex == renderPlan->clippedPortalIndex && renderPlan->nearPolygonCount) {
                        portalRenderScreenCover(renderPlan->nearPolygon, renderPlan->nearPolygonCount, current, renderState);
                    }

                    pspModelSetDepthBias(rimBias);
                    pspModelDraw(rim);
                    pspModelSetDepthBias(0);

                    sceGumPopMatrix();
                }
            } else {
                pspModelSetDepthBias(rimBias);
                portalRenderCover(portal, portalTransform, renderState);
                pspModelSetDepthBias(0);
            }
        }

        portalIndex = 1 - portalIndex;
    }
}

// Limits a portal stage to its oval, not just its rectangle: side by side
// portals' rectangles overlap diagonally.
//
// The rectangle is filled with the nearest depth, then the face (drawn from
// the parent's camera) punches the oval back to farthest, so the stage only
// lands inside it. Afterwards the rectangle is cleared to far again.
//
// Returns whether it masked, and so whether the rectangle needs clearing.
static int renderStageMaskToPortal(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* current, struct RenderState* renderState) {
    struct RenderProps* parent = current->previousProperties;

    if (!parent) {
        return 0;
    }

    int portalIndex = -1;

    for (int i = 0; i < 2; ++i) {
        if (parent->nextProperites[i] == current) {
            portalIndex = i;
        }
    }

    // Camera in the portal: the screen cover handles it, keep the rectangle.
    if (portalIndex < 0 || (parent->previousProperties == NULL && portalIndex == renderPlan->clippedPortalIndex && renderPlan->nearPolygonCount)) {
        return 0;
    }

    // Likewise when the portal crosses the parent camera's near plane: the
    // oval would miss the part in front of it.
    struct Vector3 toPortal;
    struct Vector3 forward;
    vector3Sub(&scene->portals[portalIndex].rigidBody.transform.position, &parent->camera.transform.position, &toPortal);
    quatMultVector(&parent->camera.transform.rotation, &gForward, &forward);

    // Forward is the camera's -z.
    float ahead = -vector3Dot(&toPortal, &forward);

    if (ahead - PORTAL_COVER_HEIGHT_RADIUS < parent->camera.nearPlane * (1.0f / SCENE_SCALE)) {
        return 0;
    }

    if (!cameraApplyMatrices(renderState, &parent->cameraMatrixInfo)) {
        return 0;
    }

    struct Portal* portal = &scene->portals[portalIndex];
    float portalTransform[4][4];
    portalDetermineTransform(portal, portalTransform);
    RenderMatrices matrix = renderStateMatrixFromFloat(renderState, portalTransform);

    if (!matrix) {
        return 0;
    }

    pspRenderSetScissor(current->minX, current->minY, current->maxX - current->minX, current->maxY - current->minY);

    renderViewportApply(renderViewportFullscreen());
    pspModelSetDepthBias(0);
    sceGuClearDepth(RENDER_MAX_DEPTH);
    sceGuClear(GU_DEPTH_BUFFER_BIT);

    // Face at the far depth, no colour.
    sceGuDepthRange(0, 0);
    pspWidenDepthWindow();
    sceGuDepthFunc(GU_ALWAYS);
    sceGuPixelMask(0xFFFFFFFF);

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumMultMatrix((const ScePspFMatrix4*)matrix);
    pspModelDraw((portal->flags & PortalFlagsOddParity) ? &portal_portal_blue_face_model : &portal_portal_orange_face_model);
    sceGumPopMatrix();

    sceGuPixelMask(0);
    sceGuDepthFunc(GU_GEQUAL);

    return 1;
}

void renderPlanExecute(struct RenderPlan* renderPlan, struct Scene* scene, RenderMatrices staticMatrices, struct Transform* staticTransforms, struct RenderState* renderState, struct GraphicsTask* task) {
    struct DynamicRenderDataList* dynamicList = dynamicRenderListNew(
        renderState,
        renderPlan->stageProps,
        renderPlan->stageCount,
        MAX_DYNAMIC_SCENE_OBJECTS
    );
    dynamicRenderListPopulate(dynamicList);

    renderPlanApplyLighting();

    for (int stageIndex = renderPlan->stageCount - 1; stageIndex >= 0; --stageIndex) {
        struct RenderProps* current = &renderPlan->stageProps[stageIndex];

        // How many portals deep this view is: 0 for the main view.
        int viewLevel = gSaveData.gameplay.portalRenderDepth - current->currentDepth;
        enum PspProfileBin viewBin = viewLevel <= 0 ? PspProfileView0 : viewLevel == 1 ? PspProfileView1 : PspProfileView2;
        unsigned long long stageStart = pspProfileNow();
        unsigned partsBefore = gPspProfileCounters[PspProfileParts];
        pspProfileCount(PspProfileStages, 1);

        // Off while masking: the mask is drawn from the parent's side.
        pspSetStageCullPlane(NULL);

        int masked = renderStageMaskToPortal(renderPlan, scene, current, renderState);

        if (!cameraApplyMatrices(renderState, &current->cameraMatrixInfo)) {
            // Free the list before bailing: stackMalloc() is a bump allocator and a
            // leak grows every frame until it overruns the 8KB array (into gScene).
            break;
        }

        // Drop what lies behind the exit portal; see pspSetStageCullPlane().
        if (current->previousProperties) {
            struct Plane* exitPlane = &current->cameraMatrixInfo.cullingInformation.clippingPlanes[CLIPPING_PLANE_NEAR];
            float worldPlane[4] = {exitPlane->normal.x, exitPlane->normal.y, exitPlane->normal.z, exitPlane->d};
            pspSetStageCullPlane(worldPlane);
        } else {
            pspSetStageCullPlane(NULL);
        }

        pspSetPortalView(current->previousProperties != NULL);

        renderStageApply(current);
        renderStageApplyFog(current);
        renderStageSetDepthView(current);

        // The N64's gSPLookAt: forward and up negated, set as two lights for the
        // GE's environment map.
        struct Vector3 lookS;
        struct Vector3 lookT;
        quatMultVector(&current->camera.transform.rotation, &gForward, &lookS);
        vector3Negate(&lookS, &lookS);
        quatMultVector(&current->camera.transform.rotation, &gUp, &lookT);
        vector3Negate(&lookT, &lookT);
        pspSetLookAt(&lookS, &lookT);

        renderStageDrawPortals(renderPlan, scene, current, renderState);

        unsigned long long sceneStart = pspProfileDetailNow();

        staticRender(
            current,
            dynamicList,
            stageIndex,
            staticMatrices,
            staticTransforms,
            renderState
        );

        pspProfileDetailAdd(PspProfileScene, sceneStart);

        if (current->shouldClearZBuffer || masked) {
            graphicsTaskClearZBuffer(task, current->minX, current->minY, current->maxX, current->maxY);
        }

        pspProfileAdd(viewBin, stageStart);

        if (viewLevel == 1 || viewLevel >= 2) {
            pspProfileCount(viewLevel == 1 ? PspProfilePartsView1 : PspProfilePartsView2, gPspProfileCounters[PspProfileParts] - partsBefore);
        }
    }

    pspSetStageCullPlane(NULL);
    pspSetPortalView(0);

    // The HUD, menus and gun draw after the stages, the 2D ones at depth 0,
    // which the last stage's window would discard.
    pspDepthResetWindow();
    pspWidenDepthWindow();

    dynamicRenderListFree(dynamicList);
}
