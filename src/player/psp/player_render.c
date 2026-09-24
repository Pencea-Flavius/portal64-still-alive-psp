#include "player/player.h"

#include "scene/dynamic_render_list.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/player/chell.h"
#include "codegen/assets/models/portal_gun/w_portalgun.h"

// The PSP half of the player model's drawing; see src/player/n64/player_render.c.
// The gun is a second render list entry under the attachment bone.
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

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        player->armature.displayList,
        matrix,
        DEFAULT_INDEX,
        &player->body.transform.position,
        armature,
        player->body.flags
    );

    if (!(player->flags & (PlayerHasFirstPortalGun | PlayerHasSecondPortalGun))) {
        return;
    }

    RenderMatrices gunMatrix = skAttachmentTransform(&player->armature, PLAYER_CHELL_ATTACHMENT_GUN_BONE, matrix, armature, renderState);

    if (!gunMatrix) {
        return;
    }

    dynamicRenderListAddDataTouchingPortal(
        renderList,
        portal_gun_w_portalgun_model_gfx,
        gunMatrix,
        DEFAULT_INDEX,
        &player->body.transform.position,
        NULL,
        player->body.flags
    );
}
