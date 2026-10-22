#include "particle_effect.h"

#include "math/mathf.h"
#include "math/vector2.h"
#include "physics/collision_scene.h"
#include "physics/config.h"
#include "scene/dynamic_render_list.h"
#include "scene/dynamic_scene.h"
#include "util/frame_time.h"

#define PARTICLE_EFFECT_CULLING_RADIUS 3.0f

int particleEffectIsCulled(struct ParticleEffect* effect, uint64_t* visibleRooms) {
    if (effect->cullingFlags & ParticleCullingFlagsCrossRooms) {
        effect->cullingFlags |= ParticleCullingFlagsWasSeen;

        if (ROOM_FLAG_FROM_INDEX(effect->roomIndex) & *visibleRooms) {
            effect->cullingFlags |= ParticleCullingFlagsRoomWasSeen;
        } else {
            return 1;
        }
    }

    return 0;
}

void particleEffectInit(struct ParticleEffect* effect) {
    effect->definition = NULL;
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

void particleEffectPlay(
    struct ParticleEffect* effect,
    struct ParticleEffectDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent,
    int roomIndex,
    enum ParticleCullingType cullingType
) {
    struct Vector3 right;
    struct Vector3 up;
    vector3Perp(normal, &right);
    vector3Normalize(&right, &right);
    vector3Cross(normal, &right, &up);

    for (int i = 0; i < definition->count; ++i) {
        struct Particle* particle = &effect->particles[i];

        // Compute initial velocity and position
        struct Vector2 tangentDir;
        vector2RandomUnitCircle(&tangentDir);
        float tangentMag = randomInRangef(definition->minTangentVelocity, definition->maxTangentVelocity);
        float normalMag = randomInRangef(definition->minNormalVelocity, definition->maxNormalVelocity);

        vector3Scale(normal, &particle->velocity, normalMag);
        vector3AddScaled(&particle->velocity, &right, tangentDir.x * tangentMag, &particle->velocity);
        vector3AddScaled(&particle->velocity, &up, tangentDir.y * tangentMag, &particle->velocity);

        particle->position[1] = *origin;
        vector3AddScaled(&particle->position[1], &particle->velocity, definition->tailDelay, &particle->position[0]);

        // Compute width direction (billboarded particles do this every frame)
        if (!(definition->flags & ParticleFlagsBillboarded)) {
            vector3Cross(&particle->velocity, &gUp, &particle->widthOffset);

            float widthMag = vector3MagSqrd(&particle->widthOffset);
            if (widthMag < 0.00001f) {
                vector3Scale(&gRight, &particle->widthOffset, definition->halfWidth);
            } else {
                vector3Scale(&particle->widthOffset, &particle->widthOffset, definition->halfWidth / sqrtf(widthMag));
            }
        }
    }

    effect->definition = definition;
    effect->time = 0.0f;
    effect->parent = parent;

    if (effect->parent) {
        effect->startPosition = effect->parent->position;
        effect->position = &effect->parent->position;
    } else {
        effect->startPosition = *origin;
        effect->position = &effect->startPosition;
    }

    if (effect->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(effect->dynamicId);
    }

    if (effect->definition->flags & ParticleFlagsBillboarded) {
        effect->dynamicId = dynamicSceneAddViewDependent(
            effect,
            particleEffectRenderBillboarded,
            effect->position,
            PARTICLE_EFFECT_CULLING_RADIUS
        );
    } else {
        effect->dynamicId = dynamicSceneAdd(
            effect,
            particleEffectRender,
            effect->position,
            PARTICLE_EFFECT_CULLING_RADIUS
        );
    }

    effect->roomIndex = roomIndex;
    effect->cullingFlags = 0;

    if (roomIndex != ROOM_NONE) {
        if (cullingType == ParticleCullingTypeStandard) {
            dynamicSceneSetRoomFlags(effect->dynamicId, ROOM_FLAG_FROM_INDEX(roomIndex));
        } else if (cullingType == ParticleCullingTypeCrossRooms) {
            effect->cullingFlags |= ParticleCullingFlagsCrossRooms;
        }
    }
}

static void particleEffectStop(struct ParticleEffect* effect) {
    effect->definition = NULL;

    dynamicSceneRemove(effect->dynamicId);
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

static int particleEffectUpdateCulling(struct ParticleEffect* effect) {
    if (effect->cullingFlags & ParticleCullingFlagsCrossRooms) {
        if (effect->cullingFlags & ParticleCullingFlagsRoomWasSeen) {
            // Visible from all rooms now (avoids pop out). Don't keep checking.
            effect->cullingFlags &= ~ParticleCullingFlagsCrossRooms;
        } else if (effect->cullingFlags & ParticleCullingFlagsWasSeen) {
            // Room was not visible on first viewing.
            // If the room continues to be hidden, no point showing the effect.
            // If the room becomes visible, the effect will pop in abruptly.
            // Stop to avoid performance impact or jarring graphics.
            return 0;
        } else if (effect->parent) {
            // Ensure accurate room for first viewing check
            effect->roomIndex = worldCheckDoorwayCrossings(
                gCollisionScene.world,
                &effect->startPosition,
                effect->position,
                effect->roomIndex
            );
            effect->startPosition = *effect->position;
        }
    }

    return 1;
}

void particleEffectUpdate(struct ParticleEffect* effect) {
    if (!effect->definition) {
        return;
    }

    if (!particleEffectUpdateCulling(effect)) {
        particleEffectStop(effect);
        return;
    }

    for (int i = 0; i < effect->definition->count; ++i) {
        struct Particle* particle = &effect->particles[i];

        vector3AddScaled(&particle->position[0], &particle->velocity, FIXED_DELTA_TIME, &particle->position[0]);
        vector3AddScaled(&particle->position[1], &particle->velocity, FIXED_DELTA_TIME, &particle->position[1]);

        if (!(effect->definition->flags & ParticleFlagsNoGravity)) {
            // This line simulates tracking the y-velocity of the tail
            // separately without needing to actually do so.
            //
            // tailYVelocity = yVelocity - effect->definition->tailDelay * GRAVITY_CONSTANT
            // tailPos.y = tailPos.y + tailYVelocity * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + (yVelocity - effect->definition->tailDelay * GRAVITY_CONSTANT) * FIXED_DELTA_TIME
            // tailPos.y = tailPos.y + yVelocity * FIXED_DELTA_TIME - effect->definition->tailDelay * GRAVITY_CONSTANT * FIXED_DELTA_TIME
            particle->position[1].y -= effect->definition->tailDelay * (GRAVITY_CONSTANT * FIXED_DELTA_TIME);

            particle->velocity.y += FIXED_DELTA_TIME * GRAVITY_CONSTANT;
        }
    }

    effect->time += FIXED_DELTA_TIME;

    if (effect->time >= effect->definition->lifetime) {
        particleEffectStop(effect);
    }
}
