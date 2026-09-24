#include "effects/particle_effect.h"

#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "graphics/psp/psp_vertex.h"
#include "levels/levels.h"
#include "math/mathf.h"
#include "math/vector2.h"
#include "scene/dynamic_scene.h"

#include <math.h>

// The PSP half of a particle effect's drawing; see src/effects/n64/particle_effect_render.c.
// The same quads in one index buffer; UVs normalised (see psp_vertex.h).

// The corner of the particle texture the N64 spells as (32 << 5).
#define PARTICLE_TEXEL_SPAN 32.0f

static struct PspVertexColor* particleEffectBuildQuad(
    struct PspVertexColor* vtx,
    struct Particle* particle,
    struct Vector3* position,
    unsigned int color,
    float widthScalar,
    float uSpan,
    float vSpan
) {
    for (int i = 0; i < 4; ++i, ++vtx) {
        int posIndex = i >> 1;
        int widthSign = i & 0x1;
        struct Vector3 finalPos;

        vector3AddScaled(
            &position[posIndex],
            &particle->widthOffset,
            widthSign ? widthScalar : -widthScalar,
            &finalPos
        );

        vtx->x = finalPos.x * SCENE_SCALE;
        vtx->y = finalPos.y * SCENE_SCALE;
        vtx->z = finalPos.z * SCENE_SCALE;

        vtx->u = widthSign ? 0.0f : uSpan;
        vtx->v = posIndex ? 0.0f : vSpan;

        vtx->color = color;
        vtx->padding = 0;
    }

    return vtx;
}

static void particleEffectBuildVerticesBillboarded(
    struct PspVertexColor* vtx,
    struct ParticleEffect* effect,
    unsigned int color,
    float widthScalar,
    float uSpan,
    float vSpan,
    struct Vector3* cameraPosition
) {
    for (int pidx = 0; pidx < effect->definition->count; ++pidx) {
        struct Particle* particle = &effect->particles[pidx];

        struct Vector3 tmp;
        struct Vector3 heightOffset;
        struct Vector3 position[2];

        vector3Sub(&particle->position[0], &particle->position[1], &position[0]);   // Offset
        vector3AddScaled(&particle->position[1], &position[0], 0.5f, &position[1]); // Center

        // Determine camera-facing basis for billboard
        vector3Sub(&position[1], cameraPosition, &tmp);
        vector3Cross(&tmp, &position[0], &particle->widthOffset);
        vector3Scale(&particle->widthOffset, &particle->widthOffset, effect->definition->halfWidth / sqrtf(vector3MagSqrd(&particle->widthOffset)));

        vector3Cross(&tmp, &particle->widthOffset, &heightOffset);
        vector3Scale(&heightOffset, &heightOffset, 0.5f * sqrtf(vector3MagSqrd(&position[0])) / sqrtf(vector3MagSqrd(&heightOffset)));

        // Start/end relative to center
        vector3Sub(&position[1], &heightOffset, &position[0]);
        vector3Add(&position[1], &heightOffset, &position[1]);

        vtx = particleEffectBuildQuad(vtx, particle, position, color, widthScalar, uSpan, vSpan);
    }
}

static void particleEffectBuildVertices(
    struct PspVertexColor* vtx,
    struct ParticleEffect* effect,
    unsigned int color,
    float widthScalar,
    float uSpan,
    float vSpan
) {
    for (int pidx = 0; pidx < effect->definition->count; ++pidx) {
        struct Particle* particle = &effect->particles[pidx];

        vtx = particleEffectBuildQuad(vtx, particle, particle->position, color, widthScalar, uSpan, vSpan);
    }
}

static struct PspModel* particleEffectBuildModel(struct RenderState* renderState, struct ParticleEffect* effect, struct Vector3* cameraPosition) {
    float width = 1.0f;
    if (effect->time < effect->definition->fullWidthTime) {
        width = (effect->time + 0.5f) / (effect->definition->fullWidthTime + 0.5f);
    }

    struct Coloru8 color = effect->definition->color;
    if (effect->time < effect->definition->fadeInEndTime) {
        color.a *= effect->time / effect->definition->fadeInEndTime;
    } else if (effect->time > effect->definition->fadeOutStartTime) {
        color.a *= 1.0f - mathfInvLerp(
            effect->definition->fadeOutStartTime,
            effect->definition->lifetime,
            effect->time
        );
    }

    const struct PspMaterial* material = levelMaterial(effect->definition->materialIndex);

    float uSpan = 1.0f;
    float vSpan = 1.0f;

    if (material && material->texture) {
        uSpan = PARTICLE_TEXEL_SPAN / (float)material->texture->width;
        vSpan = PARTICLE_TEXEL_SPAN / (float)material->texture->height;
    }

    int quadCount = effect->definition->count;

    struct PspVertexColor* vertices = renderStateRequestMemory(
        renderState, sizeof(struct PspVertexColor) * quadCount * 4);
    unsigned short* indices = renderStateRequestMemory(
        renderState, sizeof(unsigned short) * quadCount * 6);

    if (!vertices || !indices) {
        return NULL;
    }

    unsigned int packedColor = pspRenderColor(&color);

    if (cameraPosition) {
        particleEffectBuildVerticesBillboarded(vertices, effect, packedColor, width, uSpan, vSpan, cameraPosition);
    } else {
        particleEffectBuildVertices(vertices, effect, packedColor, width, uSpan, vSpan);
    }

    for (int i = 0; i < quadCount; ++i) {
        unsigned short base = (unsigned short)(i * 4);
        unsigned short* index = &indices[i * 6];

        index[0] = base;
        index[1] = base + 1;
        index[2] = base + 2;
        index[3] = base + 2;
        index[4] = base + 1;
        index[5] = base + 3;
    }

    return pspModelBuild(
        renderState,
        vertices, (unsigned short)(quadCount * 4),
        indices, (unsigned short)(quadCount * 6),
        PSP_VERTEX_FORMAT_COLOR,
        material
    );
}

void particleEffectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct ParticleEffect* effect = (struct ParticleEffect*)data;

    struct PspModel* model = particleEffectBuildModel(renderState, effect, NULL);

    if (!model) {
        return;
    }

    RenderMatrices matrix = NULL;

    if (effect->parent) {
        float parentMatrix[4][4];
        transformToMatrix(effect->parent, parentMatrix, SCENE_SCALE);
        matrix = renderStateMatrixFromFloat(renderState, parentMatrix);
    }

    dynamicRenderListAddData(
        renderList,
        model,
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}

void particleEffectRenderBillboarded(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct ParticleEffect* effect = (struct ParticleEffect*)data;

    struct RenderState* renderState = renderScene->renderState;

    struct PspModel* model;
    RenderMatrices matrix = NULL;

    if (effect->parent) {
        // Presence of a parent implies a parent-local particle position,
        // which necessitates a parent-local camera position as well
        struct Vector3 localCamPos;
        transformPointInverseNoScale(effect->parent, &fromView->position, &localCamPos);
        model = particleEffectBuildModel(renderState, effect, &localCamPos);

        float parentMatrix[4][4];
        transformToMatrix(effect->parent, parentMatrix, SCENE_SCALE);
        matrix = renderStateMatrixFromFloat(renderState, parentMatrix);
    } else {
        model = particleEffectBuildModel(renderState, effect, &fromView->position);
    }

    if (!model) {
        return;
    }

    renderSceneAdd(
        renderScene,
        model,
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}
