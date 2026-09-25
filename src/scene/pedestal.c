#include "pedestal.h"

#include "graphics/renderstate.h"
#include "scene/dynamic_scene.h"
#include "scene/hud.h"
#include "scene/scene.h"
#include "util/dynamic_asset_loader.h"
#include "util/frame_time.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/dynamic_animated_model_list.h"
#include "codegen/assets/models/pedestal.h"

struct Vector2 gMaxPedistalRotation;
#define MAX_PEDISTAL_ROTATION_DEGREES_PER_SEC   (M_PI / 6.0f)

static void pedestalDetermineHolderAngle(struct Pedestal* pedestal, struct Vector3* direction, struct Vector2* output) {
    output->x = direction->z;
    output->y = direction->x;
    vector2Normalize(output, output);
}

void pedestalInit(struct Pedestal* pedestal, struct PedestalDefinition* definition) {
    struct SKArmatureWithAnimations* armature = dynamicAssetAnimatedModel(PEDESTAL_DYNAMIC_ANIMATED_MODEL);

    transformInitIdentity(&pedestal->transform);

    pedestal->transform.position = definition->position;
    pedestal->roomIndex = definition->roomIndex;

    skArmatureInit(&pedestal->armature, armature->armature);

    skAnimatorInit(&pedestal->animator, PEDESTAL_DEFAULT_BONES_COUNT);

    pedestal->dynamicId = dynamicSceneAdd(pedestal, pedestalRender, &pedestal->transform.position, 0.8f);

    dynamicSceneSetRoomFlags(pedestal->dynamicId, ROOM_FLAG_FROM_INDEX(definition->roomIndex));

    pedestal->flags = 0;

    gMaxPedistalRotation.x = cosf(MAX_PEDISTAL_ROTATION_DEGREES_PER_SEC * FIXED_DELTA_TIME);
    gMaxPedistalRotation.y = sinf(MAX_PEDISTAL_ROTATION_DEGREES_PER_SEC * FIXED_DELTA_TIME);

    struct Vector3 startDir;
    quatMultVector(&definition->rotation, &gForward, &startDir);
    pedestalDetermineHolderAngle(pedestal, &startDir, &pedestal->currentRotation);
}

void pedestalUpdate(struct Pedestal* pedestal) {
    skAnimatorUpdate(&pedestal->animator, pedestal->armature.pose, FIXED_DELTA_TIME);

    if (pedestal->flags & PedestalFlagsIsPointing) {
        if (vector2RotateTowards(&pedestal->currentRotation, &pedestal->targetRotation, &gMaxPedistalRotation, &pedestal->currentRotation)) {
            if ((pedestal->flags & (PedestalFlagsDown | PedestalFlagsPlayShootingSound)) == PedestalFlagsPlayShootingSound) {
                soundPlayerPlay(soundsPedestalShooting, 2.0f, 1.0f, &pedestal->transform.position, &gZeroVec, SoundTypeAll);
                pedestal->flags &= ~PedestalFlagsPlayShootingSound;
            }
            pedestal->flags &= ~PedestalFlagsIsPointing;
        }
        else{
            if (!(pedestal->flags & PedestalFlagsAlreadyMoving) && !(pedestal->flags & PedestalFlagsDown)){
                soundPlayerPlay(soundsPedestalMoving, 2.5f, 1.0f, &pedestal->transform.position, &gZeroVec, SoundTypeAll);
                hudShowSubtitle(&gScene.hud, PORTALGUN_PEDESTAL_ROTATE, SubtitleTypeCaption);
                pedestal->flags |= PedestalFlagsAlreadyMoving;
            }
        }
        
    }
    else{
        pedestal->flags &= ~PedestalFlagsAlreadyMoving;
    }
    
    quatAxisComplex(&gUp, &pedestal->currentRotation, &pedestal->armature.pose[PEDESTAL_HOLDER_BONE].rotation);
}

void pedestalHide(struct Pedestal* pedestal) {
    soundPlayerPlay(soundsReleaseCube, 3.0f, 1.0f, &pedestal->transform.position, &gZeroVec, SoundTypeAll);
    hudShowSubtitle(&gScene.hud, WEAPON_PORTALGUN_POWERUP, SubtitleTypeCaption);
    pedestal->flags |= PedestalFlagsDown;
    skAnimatorRunClip(&pedestal->animator, dynamicAssetClip(PEDESTAL_DYNAMIC_ANIMATED_MODEL, PEDESTAL_ARMATURE_HIDE_CLIP_INDEX), 0.0f, 0);
}

void pedestalPointAt(struct Pedestal* pedestal, struct Vector3* target, int playShootingSound) {
    struct Vector3 offset;
    vector3Sub(target, &pedestal->transform.position, &offset);
    pedestalDetermineHolderAngle(pedestal, &offset, &pedestal->targetRotation);

    pedestal->flags |= PedestalFlagsIsPointing;

    if (playShootingSound) {
        pedestal->flags |= PedestalFlagsPlayShootingSound;
    }
}

// The end of the gun's barrel in the w_portalgun model, relative to the bone
// it hangs from. The model runs from -2 to -48 along y, the barrel's end last.
#define PEDESTAL_GUN_MUZZLE_Y   -48.0f

// Where a shot from the gun on the pedestal starts, turned as the holder is
// now. It used to be a fixed 0.75m above the base: the middle of the gun,
// which is where the portal's trail came out of.
void pedestalGunMuzzle(struct Pedestal* pedestal, struct Vector3* out) {
    struct Vector3 muzzle = {0.0f, PEDESTAL_GUN_MUZZLE_Y, 0.0f};
    struct Vector3 offset;
    skCalculateBonePosition(&pedestal->armature, PEDESTAL_ATTACHMENT_GUN_BONE, &muzzle, &offset);
    vector3AddScaled(&pedestal->transform.position, &offset, 1.0f / SCENE_SCALE, out);
}

void pedestalSetDown(struct Pedestal* pedestal) {
    skAnimatorRunClip(
        &pedestal->animator,
        dynamicAssetClip(PEDESTAL_DYNAMIC_ANIMATED_MODEL, PEDESTAL_ARMATURE_HIDDEN_CLIP_INDEX),
        0.0f,
        SKAnimatorStartFlagsLoadSync
    );
    skAnimatorUpdate(&pedestal->animator, pedestal->armature.pose, FIXED_DELTA_TIME);
}
