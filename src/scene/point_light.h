#ifndef __POINT_LIGHT_H__
#define __POINT_LIGHT_H__

#include "graphics/color.h"
#include "math/transform.h"
#include "math/vector3.h"

struct PointLight {
    struct Vector3 position;
    struct Coloru8 color;
    float intensity;
    float maxFactor;
};

void pointLightInit(struct PointLight* pointLight, struct Vector3* position, struct Coloru8* color, float intensity);
void pointLightSetColor(struct PointLight* pointLight, struct Coloru8* color);

// Turning a light into something the renderer can use is the platform's, and
// the build puts one half of it on the include path.
#include "point_light_render.h"

#endif