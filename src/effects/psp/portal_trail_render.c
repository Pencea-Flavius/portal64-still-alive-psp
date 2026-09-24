#include "portal_trail_render.h"

#include "effects/portal_trail.h"
#include "graphics/color.h"
#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"
#include "graphics/psp/psp_vertex.h"
#include "levels/material_state.h"
#include "levels/psp/level_materials.h"
#include "math/mathf.h"
#include "scene/camera.h"

#include <pspgu.h>
#include <pspgum.h>

#include "codegen/assets/materials/static.h"
#include "codegen/assets/models/portal_gun/ball_trail.h"

// The PSP half of the trail's drawing; see src/effects/n64/portal_trail_render.c.
//
// The trail's combiner is
//
//     colour = lerp(PRIMITIVE, 1, TEXEL0)
//     alpha  = (TEXEL0 - PRIMITIVE) * SHADE
//
// with SHADE alpha being the fog factor, 0 at the gun and 1 at the head.
// It is linear along a straight trail, so each segment gets its own alpha
// and no fog is set. The primitive alpha is the brief fade in.

void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState, struct Camera* fromCamera, int portalIndex) {
    struct PortalTrailFade fade;

    if (!portalTrailPrepareFade(trail, fromCamera, portalIndex, &fade)) {
        return;
    }

    materialStateSet(materialState, PORTAL_TRAIL_INDEX, renderState);

    struct PspMaterial* material = renderStateRequestMemory(renderState, sizeof(struct PspMaterial));

    if (!material) {
        return;
    }

    // The portal's colour and the fade in.
    struct Coloru8 tint = fade.color;
    tint.a = 255;
    float fadeIn = fade.alpha * (1.0f / 255.0f);

    *material = *portal_gun_ball_trail_model.parts[0].material;
    // Only the colour; the material does the glow.
    material->primitiveColor = pspRenderColor(&tint);

    const struct PspModelPart* segment = &portal_gun_ball_trail_model.parts[0];

    pspMaterialBind(material);

    // The same 16 bit position scale pspModelDraw() applies.
    static const ScePspFVector3 vertexScale = {
        PSP_VERTEX_POSITION_UNSCALE,
        PSP_VERTEX_POSITION_UNSCALE,
        PSP_VERTEX_POSITION_UNSCALE,
    };

    sceGumMatrixMode(GU_MODEL);
    sceGumPushMatrix();
    sceGumMultMatrix((const ScePspFMatrix4*)&trail->baseTransform[trail->currentBaseTransform]);
    sceGumScale(&vertexScale);

    float tail = fade.startDistance;
    float span = trail->lastDistance - tail;

    if (span < 0.001f) {
        span = 0.001f;
    }

    float currentDistance = fade.startDistance;
    int hasMore = 1;

    while (hasMore) {
        currentDistance += PORTAL_TRAIL_SEGMENT_LENGTH;

        hasMore = currentDistance < trail->lastDistance && currentDistance < trail->maxDistance;

        if (currentDistance <= 0.0f) {
            continue;
        }

        // The fog factor at the segment's middle, less the fade in.
        float along = (currentDistance - 0.5f * PORTAL_TRAIL_SEGMENT_LENGTH - tail) / span;
        along = along < 0.0f ? 0.0f : along > 1.0f ? 1.0f : along;
        float alpha = along * (1.0f - fadeIn);

        const void* segmentVertices = segment->vertices;

        if (segment->vertexFormat == PSP_VERTEX_FORMAT_COLOR) {
            struct PspVertexColor* shaded = renderStateRequestMemory(renderState, sizeof(struct PspVertexColor) * segment->vertexCount);

            if (shaded) {
                const struct PspVertexColor* source = segment->vertices;
                unsigned int t = material->primitiveColor;

                for (unsigned short i = 0; i < segment->vertexCount; ++i) {
                    unsigned int c = source[i].color;
                    unsigned int result = 0;

                    // The colour, as the material's fragment colour.
                    for (int shift = 0; shift < 24; shift += 8) {
                        result |= ((((c >> shift) & 0xFF) * ((t >> shift) & 0xFF)) / 255) << shift;
                    }

                    result |= (unsigned int)(((c >> 24) & 0xFF) * alpha) << 24;

                    shaded[i] = source[i];
                    shaded[i].color = result;
                }

                segmentVertices = shaded;
            }
        }

        sceGumDrawArray(
            GU_TRIANGLES,
            segment->vertexFormat,
            segment->indexCount,
            segment->indices,
            segmentVertices
        );

        if (hasMore) {
            sceGumMultMatrix((const ScePspFMatrix4*)&trail->sectionOffset);
        }
    }

    sceGumPopMatrix();
}
