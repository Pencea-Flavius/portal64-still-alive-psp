#include "particle_effect.h"

#include "math/mathf.h"
#include "math/vector2.h"
#include "physics/config.h"
#include "scene/dynamic_scene.h"
#include "util/frame_time.h"

void particleEffectInit(struct ParticleEffect* effect) {
    effect->definition = NULL;
    effect->dynamicId = INVALID_DYNAMIC_OBJECT;
}

void particleEffectPlay(
    struct ParticleEffect* effect,
    struct ParticleEffectDefinition* definition,
    struct Vector3* origin,
    struct Vector3* normal,
    struct Transform* parent
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
    effect->startPosition = *origin;
    effect->position = (effect->parent) ? &effect->parent->position : &effect->startPosition;

    if (effect->dynamicId != INVALID_DYNAMIC_OBJECT) {
        dynamicSceneRemove(effect->dynamicId);
    }

    if (effect->definition->flags & ParticleFlagsBillboarded) {
        effect->dynamicId = dynamicSceneAddViewDependent(
            effect,
            particleEffectRenderBillboarded,
            effect->position,
            3.0f
        );
    } else {
        effect->dynamicId = dynamicSceneAdd(
            effect,
            particleEffectRender,
            effect->position,
            3.0f
        );
    }
}

void particleEffectUpdate(struct ParticleEffect* effect) {
    if (!effect->definition) {
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
        effect->definition = NULL;

        dynamicSceneRemove(effect->dynamicId);
        effect->dynamicId = INVALID_DYNAMIC_OBJECT;
    }
}
