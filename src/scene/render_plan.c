#include "render_plan.h"

#include "graphics/screen_clipper.h"
#include "levels/static_render.h"
#include "levels/levels.h"
#include "math/mathf.h"
#include "math/matrix.h"
#include "physics/collision_scene.h"
#include "portal_render.h"
#include "savefile/savefile.h"
#include "scene/dynamic_scene.h"
#include "system/display.h"
#include "util/memory.h"

// if a portal takes up a small portion of the screen it is worth to clear
// the zbuffer after drawing the contents instead of 
#define PORTAL_AREA_CLEAR_THRESHOLD (100 * 100)
#define MIN_VP_WIDTH 64
#define CAMERA_CLIPPING_RADIUS  0.2f
#define PORTAL_CLIPPING_OFFSET  0.1f
#define ASPECT_SD 1.333333333333333    //  4:3
#define ASPECT_WIDE 1.777777777777778  // 16:9

void renderPropscheckViewportSize(int* min, int* max, int screenSize) {
    if (*max < MIN_VP_WIDTH) {
        *max = MIN_VP_WIDTH;
    }

    if (*min > screenSize - MIN_VP_WIDTH) {
        *min = screenSize - MIN_VP_WIDTH;
    }

    int widthGrowBy = MIN_VP_WIDTH - (*max - *min);

    if (widthGrowBy > 0) {
        *min -= widthGrowBy >> 1;
        *max += (widthGrowBy + 1) >> 1;
    }
}

int renderPropsZDistance(int currentDepth) {
    if (currentDepth >= gSaveData.gameplay.portalRenderDepth) {
        return 0;
    } else if (currentDepth < 0) {
        return RENDER_MAX_DEPTH;
    } else {
        return RENDER_MAX_DEPTH - (RENDER_MAX_DEPTH >> (gSaveData.gameplay.portalRenderDepth - currentDepth));
    }
}

RenderViewport renderPropsBuildViewport(struct RenderProps* props, struct RenderState* renderState) {
    int minX = props->minX;
    int maxX = props->maxX;
    int minY = props->minY;
    int maxY = props->maxY;

    int minZ = renderPropsZDistance(props->currentDepth);
    int maxZ = renderPropsZDistance(props->currentDepth - 1);

    renderPropscheckViewportSize(&minX, &maxX, SCREEN_WD);
    renderPropscheckViewportSize(&minY, &maxY, SCREEN_HT);

    return renderStateBuildViewport(renderState, minX, minY, maxX, maxY, minZ, maxZ);
}

void renderPropsInit(struct RenderProps* props, struct Camera* camera, float aspectRatio, struct RenderState* renderState, u16 roomIndex) {
    props->camera = *camera;
    props->aspectRatio = aspectRatio;

    cameraSetupMatrices(camera, renderState, aspectRatio, renderViewportFullscreen(), 1, &props->cameraMatrixInfo);

    props->currentDepth = gSaveData.gameplay.portalRenderDepth;
    props->exitPortalIndex = NO_PORTAL;
    props->fromRoom = roomIndex;
    props->parentStageIndex = -1;
    props->shouldClearZBuffer = 0;

    props->clippingPortalIndex = -1;

    props->minX = 0;
    props->minY = 0;
    props->maxX = SCREEN_WD;
    props->maxY = SCREEN_HT;

    props->viewport = renderPropsBuildViewport(props, renderState);

    props->previousProperties = NULL;
    props->nextProperites[0] = NULL;
    props->nextProperites[1] = NULL;

    props->portalRenderType = 0;
    props->visiblerooms = 0;
}

void renderPlanFinishView(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* properties, struct RenderState* renderState);

float getAspect()
{
    return displayGetAspect();
}

#define CALC_SCREEN_SPACE(clip_space, screen_size) ((clip_space + 1.0f) * ((screen_size) / 2))

int renderPlanPortal(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* current, int portalIndex, struct RenderProps** prevSiblingPtr, struct RenderState* renderState) {
    int exitPortalIndex = 1 - portalIndex;
    struct Portal* portal = &scene->portals[portalIndex];

    struct Vector3 forward = gForward;
    if (!(portal->flags & PortalFlagsOddParity)) {
        forward.z = -1.0f;
    }

    struct Vector3 worldForward;
    quatMultVector(&portal->rigidBody.transform.rotation, &forward, &worldForward);

    struct Vector3 offsetFromCamera;
    vector3Sub(&current->camera.transform.position, &portal->rigidBody.transform.position, &offsetFromCamera);

    // don't render the portal if it is facing the wrong way
    if (vector3Dot(&worldForward, &offsetFromCamera) < 0.0f) {
        return 0;
    }
    
    int flags = PORTAL_RENDER_TYPE_VISIBLE(portalIndex);

    float portalTransform[4][4];

    portalDetermineTransform(portal, portalTransform);

    if (current->currentDepth == 0 || !collisionSceneIsPortalOpen() || renderPlan->stageCount >= MAX_PORTAL_STEPS) {
        return flags; 
    }

    struct RenderProps* next = &renderPlan->stageProps[renderPlan->stageCount];

    struct ScreenClipper clipper;

    screenClipperInitWithCamera(&clipper, &current->camera, getAspect(), portalTransform);
    struct Box2D clippingBounds;
    screenClipperBoundingPoints(&clipper, gPortalOutline, sizeof(gPortalOutline) / sizeof(*gPortalOutline), &clippingBounds);

    if (clipper.nearPolygonCount) {
        renderPlan->nearPolygonCount = clipper.nearPolygonCount;
        memCopy(renderPlan->nearPolygon, clipper.nearPolygon, sizeof(struct Vector2s16) * clipper.nearPolygonCount);
        renderPlan->clippedPortalIndex = portalIndex;
    }

#ifdef PSP
    // Rounded outwards, and the parent's whole rectangle when the camera is in
    // the portal, so no gap is left at the screen's edge.
    if (clipper.nearPolygonCount) {
        next->minX = current->minX;
        next->maxX = current->maxX;
        next->minY = current->minY;
        next->maxY = current->maxY;
    } else {
        next->minX = (int)floorf(CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD));
        next->maxX = (int)ceilf(CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD));
        next->minY = (int)floorf(CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT));
        next->maxY = (int)ceilf(CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT));
    }
#else
    next->minX = CALC_SCREEN_SPACE(clippingBounds.min.x, SCREEN_WD);
    next->maxX = CALC_SCREEN_SPACE(clippingBounds.max.x, SCREEN_WD);
    next->minY = CALC_SCREEN_SPACE(-clippingBounds.max.y, SCREEN_HT);
    next->maxY = CALC_SCREEN_SPACE(-clippingBounds.min.y, SCREEN_HT);
#endif

    next->minX = MAX(next->minX, current->minX);
    next->maxX = MIN(next->maxX, current->maxX);
    next->minY = MAX(next->minY, current->minY);
    next->maxY = MIN(next->maxY, current->maxY);

    struct RenderProps* prevSibling = prevSiblingPtr ? *prevSiblingPtr : NULL;

#ifdef PSP
    // The PSP masks each stage to its oval instead (renderStageMaskToPortal()).
    prevSibling = NULL;
#endif

    if (prevSibling) {
        int topDiff = 0;
        int bottomDiff = 0;
        int leftDiff = 0;
        int rightDiff = 0;

        // if top left overlapping
        if ((next->minX > prevSibling->minX) && (next->minX < prevSibling->maxX) && (next->minY > prevSibling->minY) && (next->minY < prevSibling->maxY)){
            topDiff = MAX(abs(prevSibling->maxY-next->minY), topDiff);
            leftDiff = MAX(abs(prevSibling->maxX-next->minX), leftDiff);
        }

        // if top right overlapping
        if ((next->maxX > prevSibling->minX) && (next->maxX < prevSibling->maxX) && (next->minY > prevSibling->minY) && (next->minY < prevSibling->maxY)){
            topDiff = MAX(abs(prevSibling->maxY-next->minY), topDiff);
            rightDiff = MAX(abs(next->maxX -prevSibling->minX), rightDiff);
        }

        // if bottom left overlapping
        if ((next->minX > prevSibling->minX) && (next->minX < prevSibling->maxX) && (next->maxY > prevSibling->minY) && (next->maxY < prevSibling->maxY)){
            bottomDiff = MAX(abs(next->maxY-prevSibling->minY), bottomDiff);
            leftDiff = MAX(abs(prevSibling->maxX-next->minX), leftDiff);
        }

        // if bottom right overlapping
        if ((next->maxX > prevSibling->minX) && (next->maxX < prevSibling->maxX) && (next->maxY > prevSibling->minY) && (next->maxY < prevSibling->maxY)){
            bottomDiff = MAX(abs(next->maxY-prevSibling->minY), bottomDiff);
            rightDiff = MAX(abs(next->maxX -prevSibling->minX), rightDiff);
        }

        //shouldnt draw at all if fully overlapped
        if (rightDiff && leftDiff && topDiff && bottomDiff){
            return 0;
        }
        //cut nothing if no overlap
        else if (!rightDiff && !leftDiff && !topDiff && !bottomDiff){
            //do nothing
        }
        //should only cut out top portion
        else if (rightDiff && leftDiff && topDiff && !bottomDiff){
            next->minY += topDiff;
        }
        //should only cut out bottom portion
        else if (rightDiff && leftDiff && !topDiff && bottomDiff){
            next->maxY -= bottomDiff;
        }
        //should only cut out right portion
        else if (rightDiff && !leftDiff && topDiff && bottomDiff){
            next->maxX -= rightDiff;
        }
        //should only cut out left portion
        else if (!rightDiff && leftDiff && topDiff && bottomDiff){
            next->minX += leftDiff;
        }
        // only one corner is overlapping so cut the larger side
        else{
            if (MAX(rightDiff, leftDiff) > MAX(topDiff, bottomDiff)){
                next->minY += topDiff;
                next->maxY -= bottomDiff;
            }
            else{
                next->minX += leftDiff;
                next->maxX -= rightDiff;
            }
        }
    }

    if (next->minX >= next->maxX || next->minY >= next->maxY) {
        return 0;
    }

    struct Transform* fromPortal = &scene->portals[portalIndex].rigidBody.transform;
    struct Transform* exitPortal = &scene->portals[exitPortalIndex].rigidBody.transform;

    struct Transform otherInverse;
    transformInvert(fromPortal, &otherInverse);
    struct Transform portalCombined;
    transformConcat(exitPortal, &otherInverse, &portalCombined);

    next->camera = current->camera;
    next->camera.farPlane = DEFAULT_FAR_PLANE * SCENE_SCALE;
    next->camera.nearPlane = DEFAULT_NEAR_PLANE * SCENE_SCALE;
    next->aspectRatio = current->aspectRatio;
    transformConcat(&portalCombined, &current->camera.transform, &next->camera.transform);

    struct Vector3 portalOffset;
    vector3Sub(&exitPortal->position, &next->camera.transform.position, &portalOffset);

    struct Vector3 cameraForward;
    quatMultVector(&next->camera.transform.rotation, &gForward, &cameraForward);

    // Decrease the near clipping plane (for rendering) so objects touching portals aren't cut off
    // In the worst case, the camera is looking across the portal from one of its ends
    next->camera.nearPlane = (-vector3Dot(&portalOffset, &cameraForward)) * SCENE_SCALE - SCENE_SCALE * PORTAL_COVER_HEIGHT_RADIUS;

    if (next->camera.nearPlane < current->camera.nearPlane) {
        next->camera.nearPlane = current->camera.nearPlane;

        if (next->camera.nearPlane > next->camera.farPlane) {
            next->camera.nearPlane = next->camera.farPlane;
        }
    }

    next->shouldClearZBuffer = (next->maxX - next->minX) * (next->maxY - next->minY) < PORTAL_AREA_CLEAR_THRESHOLD;

    next->currentDepth = current->currentDepth - 1;
    next->viewport = renderPropsBuildViewport(next, renderState);

    if (!next->viewport) {
        return flags;
    }

    if (!cameraSetupMatrices(&next->camera, renderState, next->aspectRatio, next->viewport, 1, &next->cameraMatrixInfo)) {
        return flags;
    }

    // Set the near clipping plane (for culling) to be the exit portal surface
    struct Plane* nearPlane = &next->cameraMatrixInfo.cullingInformation.clippingPlanes[CLIPPING_PLANE_NEAR];
    quatMultVector(&exitPortal->rotation, &gForward, &nearPlane->normal);
    if (portalIndex == 1) {
        vector3Negate(&nearPlane->normal, &nearPlane->normal);
    }
    nearPlane->d = -(vector3Dot(&nearPlane->normal, &exitPortal->position) + 0.01f) * SCENE_SCALE;

    next->clippingPortalIndex = -1;

    next->exitPortalIndex = exitPortalIndex;
    next->fromRoom = gCollisionScene.portalRooms[next->exitPortalIndex];
    next->parentStageIndex = current - renderPlan->stageProps;

    ++renderPlan->stageCount;

    next->previousProperties = current;
    current->nextProperites[portalIndex] = next;
    next->nextProperites[0] = NULL;
    next->nextProperites[1] = NULL;

    next->visiblerooms = 0;

    next->portalRenderType = 0;

    *prevSiblingPtr = next;

    return flags | PORTAL_RENDER_TYPE_ENABLED(portalIndex);
}

#define MIN_FAR_PLANE   (5.0f * SCENE_SCALE)
#define FAR_PLANE_EXTRA  2.0f

// Geometry can reach past its room's box (a shaft below a grating), so the
// far plane also takes each visible room's static boxes.
static float renderPlanStaticMaxDistance(struct Ray* ray, u64 roomMask) {
    float result = 0.0f;

    for (int room = 0; room < gCurrentLevel->world.roomCount; ++room) {
        if (!((1LL << room) & roomMask)) {
            continue;
        }

        struct StaticIndex* index = &gCurrentLevel->roomBvhList[room];

        for (int i = 0; i < index->boxCount; ++i) {
            struct BoundingBoxs16* box = &index->boxIndex[i].box;
            struct Vector3 corner;
            corner.x = (ray->dir.x > 0.0f ? box->maxX : box->minX) * (1.0f / SCENE_SCALE);
            corner.y = (ray->dir.y > 0.0f ? box->maxY : box->minY) * (1.0f / SCENE_SCALE);
            corner.z = (ray->dir.z > 0.0f ? box->maxZ : box->minZ) * (1.0f / SCENE_SCALE);

            result = MAX(result, rayDetermineDistance(ray, &corner));
        }
    }

    return result;
}

void renderPlanDetermineFarPlane(struct Ray* cameraRay, struct RenderProps* properties) {
    float roomDistance = worldMaxDistanceInDirection(&gCurrentLevel->world, cameraRay, properties->visiblerooms);
    float staticDistance = renderPlanStaticMaxDistance(cameraRay, properties->visiblerooms);

    float furthestDistance = (MAX(roomDistance, staticDistance) + FAR_PLANE_EXTRA) * SCENE_SCALE;

    if (furthestDistance < MIN_FAR_PLANE) {
        properties->camera.farPlane = MIN_FAR_PLANE;
    } else {
        properties->camera.farPlane = MIN(properties->camera.farPlane, furthestDistance);
    }
}

int renderShouldRenderPortal(struct Scene* scene, int visiblePortal, struct RenderProps* properties) {
    if (!gCollisionScene.portalTransforms[visiblePortal]) {
        return 0;
    }

    if ((scene->player.body.flags & (RigidBodyIsTouchingPortal0 << visiblePortal)) && properties->currentDepth == gSaveData.gameplay.portalRenderDepth) {
        return 1;
    }

    struct Portal* portal = &scene->portals[visiblePortal];
    struct BoundingBoxs16 portalBox;
    portalBox.minX = (s16)(portal->collisionObject.boundingBox.min.x * SCENE_SCALE);
    portalBox.minY = (s16)(portal->collisionObject.boundingBox.min.y * SCENE_SCALE);
    portalBox.minZ = (s16)(portal->collisionObject.boundingBox.min.z * SCENE_SCALE);

    portalBox.maxX = (s16)(portal->collisionObject.boundingBox.max.x * SCENE_SCALE);
    portalBox.maxY = (s16)(portal->collisionObject.boundingBox.max.y * SCENE_SCALE);
    portalBox.maxZ = (s16)(portal->collisionObject.boundingBox.max.z * SCENE_SCALE);

    if (isOutsideFrustum(&properties->cameraMatrixInfo.cullingInformation, &portalBox) == FrustumResultOutside) {
        return 0;
    }

    // The coarse bounding box can clip through the back of the other portal.
    // Check more precisely when in a child view to avoid showing from behind.
    if (properties->currentDepth < gSaveData.gameplay.portalRenderDepth) {
        struct Basis* portalBasis = &portal->rigidBody.rotationBasis;
        struct Plane* nearPlane = &properties->cameraMatrixInfo.cullingInformation.clippingPlanes[CLIPPING_PLANE_NEAR];
        struct Vector3 closestPoint;

        vector3AddScaled(
            &gCollisionScene.portalTransforms[visiblePortal]->position,
            &portalBasis->x,
            signf(vector3Dot(&nearPlane->normal, &portalBasis->x)) * PORTAL_COVER_WIDTH_RADIUS,
            &closestPoint
        );
        vector3AddScaled(
            &closestPoint,
            &portalBasis->y,
            signf(vector3Dot(&nearPlane->normal, &portalBasis->y)) * PORTAL_COVER_HEIGHT_RADIUS,
            &closestPoint
        );
        vector3Scale(&closestPoint, &closestPoint, SCENE_SCALE);

        return planePointDistance(nearPlane, &closestPoint) >= 0.0f;
    }

    return 1;
}

void renderPlanFinishView(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderProps* properties, struct RenderState* renderState) {
    struct FrustumCullingInformation* cullingInfo = &properties->cameraMatrixInfo.cullingInformation;
    u64 coveredDoorways = 0;
    sceneGetCoveredDoorways(scene, &cullingInfo->cameraPos, &coveredDoorways);
    staticRenderDetermineVisibleRooms(properties, cullingInfo, properties->fromRoom, 0, &coveredDoorways);

    if (scene->hideCurrentRoom) {
        properties->visiblerooms &= ~(1LL << properties->fromRoom);
    }

    struct Ray cameraRay;
    quatMultVector(&properties->camera.transform.rotation, &gForward, &cameraRay.dir);
    cameraRay.origin = properties->camera.transform.position;
    vector3Negate(&cameraRay.dir, &cameraRay.dir);

    renderPlanDetermineFarPlane(&cameraRay, properties);

    cameraSetupMatrices(&properties->camera, renderState, properties->aspectRatio, properties->viewport, 0, &properties->cameraMatrixInfo);

    int closerPortal = vector3DistSqrd(&properties->camera.transform.position, &scene->portals[0].rigidBody.transform.position) < vector3DistSqrd(&properties->camera.transform.position, &scene->portals[1].rigidBody.transform.position) ? 0 : 1;
    int otherPortal = 1 - closerPortal;
    
    if (closerPortal) {
        properties->portalRenderType |= PORTAL_RENDER_TYPE_SECOND_CLOSER;
    }

    struct RenderProps* prevSibling = NULL;

    int childrenNeedZBuffer = 0;

    for (int i = 0; i < 2; ++i) {
        if (properties->exitPortalIndex != closerPortal && 
            staticRenderIsRoomVisible(properties->visiblerooms, gCollisionScene.portalRooms[closerPortal]) &&
            renderShouldRenderPortal(scene, closerPortal, properties)
        ) {

            int planResult = renderPlanPortal(
                renderPlan,
                scene,
                properties,
                closerPortal,
                &prevSibling,
                renderState
            );

            properties->portalRenderType |= planResult;

            if (planResult & PORTAL_RENDER_TYPE_ENABLED(closerPortal)) {
                renderPlanFinishView(renderPlan, scene, prevSibling, renderState);

                if (!prevSibling->shouldClearZBuffer) {
                    childrenNeedZBuffer = 1;
                }
            }
        }

        closerPortal = 1 - closerPortal;
        otherPortal = 1 - otherPortal;
    }

    if (childrenNeedZBuffer) {
        properties->shouldClearZBuffer = 0;
    }
}

void renderPlanAdjustViewportDepth(struct RenderPlan* renderPlan) {
    float depthWeight[gSaveData.gameplay.portalRenderDepth + 1];

    for (int i = 0; i <= gSaveData.gameplay.portalRenderDepth; ++i) {
        depthWeight[i] = 0.0f;
    }

    for (int i = 0; i < renderPlan->stageCount; ++i) {
        struct RenderProps* current = &renderPlan->stageProps[i];

        if (current->shouldClearZBuffer) {
            continue;
        }

        float depth = current->camera.farPlane - current->camera.nearPlane;

        depthWeight[current->currentDepth] = MAX(depthWeight[current->currentDepth], depth);
    }

    float totalWeight = 0.0f;

    for (int i = 0; i <= gSaveData.gameplay.portalRenderDepth; ++i) {
        totalWeight += depthWeight[i];
    }

    // give the main view a larger slice of the depth buffer
    totalWeight += depthWeight[gSaveData.gameplay.portalRenderDepth];
    depthWeight[gSaveData.gameplay.portalRenderDepth] *= 2.0f;

    float scale = (float)RENDER_MAX_DEPTH / totalWeight;

    // int: the GE's 16 bit depth does not fit a signed short.
    int zBufferBoundary[gSaveData.gameplay.portalRenderDepth + 2];

    zBufferBoundary[gSaveData.gameplay.portalRenderDepth + 1] = 0;

    for (int i = gSaveData.gameplay.portalRenderDepth; i >= 0; --i) {
        zBufferBoundary[i] = (int)(scale * depthWeight[i]) + zBufferBoundary[i + 1];

        zBufferBoundary[i] = MIN(zBufferBoundary[i], RENDER_MAX_DEPTH);
    }

    for (int i = 0; i < renderPlan->stageCount; ++i) {
        struct RenderProps* current = &renderPlan->stageProps[i];
        int useDepth = current->currentDepth;

        struct RenderProps* depthSearch = current;

        while (depthSearch && depthSearch->shouldClearZBuffer) {
            depthSearch = &renderPlan->stageProps[depthSearch->parentStageIndex];
        }

        if (depthSearch) {
            useDepth = depthSearch->currentDepth;
        }

        int minZ = zBufferBoundary[useDepth + 1];
        int maxZ = zBufferBoundary[useDepth];

        renderViewportSetDepthRange(current->viewport, minZ, maxZ);
    }
}

void renderPlanBuild(struct RenderPlan* renderPlan, struct Scene* scene, struct RenderState* renderState) {
    renderPropsInit(&renderPlan->stageProps[0], &scene->camera, getAspect(), renderState, scene->player.body.currentRoom);
    renderPlan->stageCount = 1;
    renderPlan->clippedPortalIndex = -1;
    renderPlan->nearPolygonCount = 0;

    renderPlanFinishView(renderPlan, scene, &renderPlan->stageProps[0], renderState);

    renderPlanAdjustViewportDepth(renderPlan);
}
