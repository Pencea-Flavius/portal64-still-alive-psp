#include "scene/render_plan.h"

#include "graphics.h"

#include "levels/static_render.h"
#include "math/mathf.h"
#include "scene/dynamic_scene.h"
#include "scene/portal_render.h"
#include "scene/scene.h"
#include "system/display.h"

#include "codegen/assets/models/portal/portal_blue.h"
#include "codegen/assets/models/portal/portal_blue_face.h"
#include "codegen/assets/models/portal/portal_orange.h"
#include "codegen/assets/models/portal/portal_orange_face.h"

#define MIN_FOG_DISTANCE 1.0f
#define MAX_FOG_DISTANCE 2.5f

extern LookAt gLookAt;

void renderPlanExecute(struct RenderPlan* renderPlan, struct Scene* scene, RenderMatrices staticMatrices, struct Transform* staticTransforms, struct RenderState* renderState, struct GraphicsTask* task) {
    struct DynamicRenderDataList* dynamicList = dynamicRenderListNew(
        renderState,
        renderPlan->stageProps,
        renderPlan->stageCount,
        MAX_DYNAMIC_SCENE_OBJECTS
    );
    dynamicRenderListPopulate(dynamicList);

    for (int stageIndex = renderPlan->stageCount - 1; stageIndex >= 0; --stageIndex) {
        struct RenderProps* current = &renderPlan->stageProps[stageIndex];

        if (!cameraApplyMatrices(renderState, &current->cameraMatrixInfo)) {
            return;
        }

        gSPViewport(renderState->dl++, current->viewport);
        gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, current->minX, current->minY, current->maxX, current->maxY);

        float lerpMin = cameraClipDistance(&current->camera, MIN_FOG_DISTANCE);
        float lerpMax = cameraClipDistance(&current->camera, MAX_FOG_DISTANCE);

        int fogMin = fogIntValue(lerpMin);
        int fogMax = fogIntValue(lerpMax);

        if (fogMax <= 0) {
            fogMax = 1;
        }

        if (fogMin >= 1000) {
            fogMin = 999;
        }
        
        gSPFogPosition(renderState->dl++, fogMin, fogMax);

        // this lookat calcuation only takes into account 
        // the direction of the camera. A better approach would
        // be to take into account the direction towards each
        // reflective object from the camera. fixing this could
        // come later
        LookAt* lookAt = renderStateRequestLookAt(renderState);
        *lookAt = gLookAt;
        struct Vector3 cameraForward;
        quatMultVector(&current->camera.transform.rotation, &gForward, &cameraForward);
        vector3Negate(&cameraForward, &cameraForward);
        vector3ToVector3u8(&cameraForward, (struct Vector3u8*)&lookAt->l[0].l.dir);

        quatMultVector(&current->camera.transform.rotation, &gUp, &cameraForward);
        vector3Negate(&cameraForward, &cameraForward);
        vector3ToVector3u8(&cameraForward, (struct Vector3u8*)&lookAt->l[1].l.dir);
        gSPLookAt(renderState->dl++, lookAt);

        int portalIndex = (current->portalRenderType & PORTAL_RENDER_TYPE_SECOND_CLOSER) ? 1 : 0;
        
        for (int i = 0; i < 2; ++i) {
            if (current->portalRenderType & PORTAL_RENDER_TYPE_VISIBLE(portalIndex)) {
                float portalTransform[4][4];
                struct Portal* portal = &scene->portals[portalIndex];
                portalDetermineTransform(portal, portalTransform);

                struct RenderProps* portalProps = current->nextProperites[portalIndex];

                if (portalProps && current->portalRenderType & PORTAL_RENDER_TYPE_ENABLED(portalIndex)) {
                    // render the front portal cover
                    Mtx* matrix = renderStateRequestMatrices(renderState, 1);

                    if (!matrix) {
                        continue;;
                    }

                    guMtxF2L(portalTransform, matrix);
                    gSPMatrix(renderState->dl++, matrix, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_MUL);

                    gDPSetEnvColor(renderState->dl++, 255, 255, 255, portal->opacity < 0.0f ? 0 : (portal->opacity > 1.0f ? 255 : (u8)(portal->opacity * 255.0f)));
                    
                    Gfx* faceModel;
                    Gfx* portalModel;

                    if (portal->flags & PortalFlagsOddParity) {
                        faceModel = portal_portal_blue_face_model_gfx;
                        portalModel = portal_portal_blue_model_gfx;
                    } else {
                        faceModel = portal_portal_orange_face_model_gfx;
                        portalModel = portal_portal_orange_model_gfx;
                    }
                    
                    if (portal->flags & PortalFlagsZOffset) {
                        // render the portal cover with a slightly offset z
                        // so it doesn't z fight with the surface it is attached to
                        Vp* vpWithOffset = renderStateRequestViewport(renderState);
                        *vpWithOffset = *current->viewport;
                        vpWithOffset->vp.vtrans[2] -= 2;
                        gSPViewport(renderState->dl++, vpWithOffset);
                    }
                    gSPDisplayList(renderState->dl++, faceModel);
                    gSPViewport(renderState->dl++, current->viewport);
                    if (current->previousProperties == NULL && portalIndex == renderPlan->clippedPortalIndex && renderPlan->nearPolygonCount) {
                        portalRenderScreenCover(renderPlan->nearPolygon, renderPlan->nearPolygonCount, current, renderState);
                    }
                    gDPPipeSync(renderState->dl++);

                    gSPDisplayList(renderState->dl++, portalModel);
                    
                    gSPPopMatrix(renderState->dl++, G_MTX_MODELVIEW);
                } else {
                    portalRenderCover(portal, portalTransform, renderState);
                }
            }

            portalIndex = 1 - portalIndex;
        }

        staticRender(
            current,
            dynamicList,
            stageIndex,
            staticMatrices,
            staticTransforms,
            renderState
        );

        if (current->shouldClearZBuffer) {
            graphicsTaskClearZBuffer(task, current->minX, current->minY, current->maxX, current->maxY);
        }
    }

    dynamicRenderListFree(dynamicList);
}