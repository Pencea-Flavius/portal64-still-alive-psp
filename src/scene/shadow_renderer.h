#ifndef __SHADOW_RENDERER_H__
#define __SHADOW_RENDERER_H__

#include "graphics/render_types.h"

#include <ultra64.h>

#include "graphics/color.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "math/vector2.h"
#include "point_light.h"

enum ShadowReceiverFlags {
    ShadowReceiverFlagsUseLight = (1 << 0),
};

struct ShadowReceiver {
    RenderDisplayList litMaterial;
    RenderDisplayList shadowMaterial;
    ModelHandle geometry;
    unsigned short flags;
    struct Transform transform;
};

struct ShadowRenderer {
    RenderDisplayList shadowVolume;
    RenderDisplayList shadowProfile;
    RenderVertices vertices;
    struct Transform casterTransform;
    float shadowLength;
};

void shadowRendererInit(struct ShadowRenderer* shadowRenderer, struct Vector2* outline, unsigned pointCount, float shadowLength);
void shadowRendererRender(
    struct ShadowRenderer* shadowRenderer, 
    struct RenderState* renderState, 
    struct PointLight* fromLight,
    struct ShadowReceiver* recievers, 
    unsigned recieverCount
);
void shadowRendererRenderProjection(
    struct ShadowRenderer* shadowRenderer, 
    struct RenderState* renderState, 
    struct PointLight* fromLight,
    struct Vector3* toPoint,
    struct Vector3* normal
);

#endif