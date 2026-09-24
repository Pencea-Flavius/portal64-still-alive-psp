#include "effects/particle_effect.h"

#include "graphics/renderstate.h"
#include "math/mathf.h"
#include "math/vector2.h"
#include "scene/dynamic_scene.h"

static Vtx* particleEffectBuildQuad(
    Vtx* vtx,
    struct Particle* particle,
    struct Vector3* position,
    struct Coloru8* color,
    float widthScalar
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

        vtx->v.ob[0] = finalPos.x * SCENE_SCALE;
        vtx->v.ob[1] = finalPos.y * SCENE_SCALE;
        vtx->v.ob[2] = finalPos.z * SCENE_SCALE;

        vtx->v.flag = 0;
        vtx->v.tc[0] = widthSign ? 0 : (32 << 5);
        vtx->v.tc[1] = posIndex ? 0 : (32 << 5);

        vtx->v.cn[0] = color->r;
        vtx->v.cn[1] = color->g;
        vtx->v.cn[2] = color->b;
        vtx->v.cn[3] = color->a;
    }

    return vtx;
}

static void particleEffectBuildVerticesBillboarded(
    Vtx* vtx,
    struct ParticleEffect* effect,
    struct Coloru8* color,
    float widthScalar,
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

        vtx = particleEffectBuildQuad(vtx, particle, position, color, widthScalar);
    }
}

static void particleEffectBuildVertices(Vtx* vtx, struct ParticleEffect* effect, struct Coloru8* color, float widthScalar) {
    for (int pidx = 0; pidx < effect->definition->count; ++pidx) {
        struct Particle* particle = &effect->particles[pidx];

        vtx = particleEffectBuildQuad(vtx, particle, particle->position, color, widthScalar);
    }
}

static Gfx* particleEffectBuildDisplayList(struct RenderState* renderState, struct ParticleEffect* effect, struct Vector3* cameraPosition) {
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

    // Build quads
    Vtx* vertices = renderStateRequestVertices(renderState, effect->definition->count * 4);
    if (cameraPosition) {
        particleEffectBuildVerticesBillboarded(vertices, effect, &color, width, cameraPosition);
    } else {
        particleEffectBuildVertices(vertices, effect, &color, width);
    }

    // Render quads (can load 32 vertices/8 quads at once)
    Gfx* displayList = renderStateAllocateDLChunk(
        renderState,
        effect->definition->count + ((effect->definition->count + 7) >> 3) + 1
    );
    Gfx* dl = displayList;

    for (int i = 0; i < effect->definition->count; ++i) {
        int relativeVertex = (i << 2) & 0x1f;

        if (relativeVertex == 0) {
            // Load next batch of vertices
            int verticesLeft = MIN(32, (effect->definition->count - i) << 2);
            gSPVertex(dl++, &vertices[i << 2], verticesLeft, 0);
        }

        gSP2Triangles(
            dl++,
            relativeVertex,
            relativeVertex + 1,
            relativeVertex + 2,
            0,
            relativeVertex + 2,
            relativeVertex + 1,
            relativeVertex + 3,
            0
        );
    }

    gSPEndDisplayList(dl++);

    return displayList;
}

void particleEffectRender(void* data, struct DynamicRenderDataList* renderList, struct RenderState* renderState) {
    struct ParticleEffect* effect = (struct ParticleEffect*)data;

    Mtx* matrix = NULL;
    if (effect->parent) {
        matrix = renderStateRequestMatrices(renderState, 1);
        transformToMatrixL(effect->parent, matrix, SCENE_SCALE);
    }

    dynamicRenderListAddData(
        renderList,
        particleEffectBuildDisplayList(renderState, effect, NULL),
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}

void particleEffectRenderBillboarded(void* data, struct RenderScene* renderScene, struct Transform* fromView) {
    struct ParticleEffect* effect = (struct ParticleEffect*)data;

    Gfx* gfx;
    Mtx* matrix = NULL;

    if (effect->parent) {
        // Presence of a parent implies a parent-local particle position,
        // which necessitates a parent-local camera position as well
        struct Vector3 localCamPos;
        transformPointInverseNoScale(effect->parent, &fromView->position, &localCamPos);
        gfx = particleEffectBuildDisplayList(renderScene->renderState, effect, &localCamPos);

        matrix = renderStateRequestMatrices(renderScene->renderState, 1);
        transformToMatrixL(effect->parent, matrix, SCENE_SCALE);
    } else {
        gfx = particleEffectBuildDisplayList(renderScene->renderState, effect, &fromView->position);
    }

    renderSceneAdd(
        renderScene,
        gfx,
        matrix,
        effect->definition->materialIndex,
        effect->position,
        NULL
    );
}
