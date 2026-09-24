#include "player/player.h"

#include "scene/dynamic_render_list.h"
#include "sk64/skeletool_defs.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/player/chell.h"
#include "codegen/assets/models/portal_gun/w_portalgun.h"

// The N64 half of the player model's drawing; see src/player/psp/player_render.c.
// The gun is an attachment spliced in through the bone attachment segment.

void playerRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct Player* player = (struct Player*)data;

    struct Transform finalPlayerTransform;

    struct Vector3 forwardVector;
    struct Vector3 unusedRight;

    playerGetMoveBasis(&player->lookTransform.rotation, &forwardVector, &unusedRight);

    finalPlayerTransform.position = player->body.transform.position;
    quatLook(&forwardVector, &gUp, &finalPlayerTransform.rotation);
    finalPlayerTransform.scale = gOneVec;

    // Feet on the floor, also when crouched.
    finalPlayerTransform.position.y -= playerStandHeight(player);

    RenderMatrices matrix = renderStateTransformToMatrices(renderState, &finalPlayerTransform, SCENE_SCALE);

    if (!matrix) {
        return;
    }

    RenderMatrices armature = skArmatureBuildTransforms(&player->armature, renderState);

    if (!armature) {
        return;
    }

    Gfx* gunGfx = portal_gun_w_portalgun_model_gfx;
    Gfx** gunAttachment = (player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun))
        ? &gunGfx
        : NULL;
    Gfx* attachments = skBuildAttachments(&player->armature, gunAttachment, renderState);

    Gfx* objectRender = renderStateAllocateDLChunk(renderState, 4);
    Gfx* dl = objectRender;

    if (attachments) {
        gSPSegment(dl++, BONE_ATTACHMENT_SEGMENT,  osVirtualToPhysical(attachments));
    }
    gSPSegment(dl++, MATRIX_TRANSFORM_SEGMENT,  osVirtualToPhysical(armature));
    gSPDisplayList(dl++, player->armature.displayList);
    gSPEndDisplayList(dl++);


    dynamicRenderListAddDataTouchingPortal(
        renderList,
        objectRender,
        matrix,
        DEFAULT_INDEX,
        &player->body.transform.position,
        armature,
        player->body.flags
    );
}

