#ifndef __SCENE_N64_POINT_LIGHT_RENDER_H__
#define __SCENE_N64_POINT_LIGHT_RENDER_H__

// The N64 half of point lights; see src/scene/psp/point_light_render.h.
// The RSP takes directional lights, so a point light is attenuated on the
// CPU and handed over as a Light.

#include <ultra64.h>

#include "graphics/color.h"
#include "graphics/renderstate.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "scene/point_light.h"

struct PointLightableMesh {
    struct Vector3* vertexNormals;
    struct Vector3* vertexTangents;
    struct Vector3* vertexBitangents;
    Vtx* inputVertices;
    Vtx* oututVertices;
    Gfx* drawCommand;
    unsigned vertexCount;
    struct Coloru8 color;
};

extern Light gLightBlack;

void pointLightCalculateLight(struct PointLight* pointLight, struct Vector3* target, Light* output);

void pointLightableSetMaterial(struct PointLightableMesh* mesh, struct RenderState* renderState, struct Coloru8* ambient);
void pointLightableSetMaterialInShadow(struct PointLightableMesh* mesh, struct RenderState* renderState, struct Coloru8* ambient);
void pointLightableMeshInit(struct PointLightableMesh* mesh, Vtx* inputVertices, Gfx* drawCommand, struct Coloru8* color);
void pointLightableCalc(struct PointLightableMesh* mesh, struct Transform* meshTransform, struct PointLight* pointLight);

#endif
