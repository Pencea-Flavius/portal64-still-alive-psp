
#include "math/mathf.h"
#include "point_light.h"
#include "util/memory.h"

void pointLightRecalcMaxFactor(struct PointLight* pointLight) {
    float result = 10000000.0f;
    
    if (pointLight->color.r) {
        result = MIN(result, 255.0f / pointLight->color.r);
    }

    if (pointLight->color.g) {
        result = MIN(result, 255.0f / pointLight->color.g);
    }

    if (pointLight->color.b) {
        result = MIN(result, 255.0f / pointLight->color.b);
    }

    pointLight->maxFactor = result;
}

void pointLightInit(struct PointLight* pointLight, struct Vector3* position, struct Coloru8* color, float intensity) {
    pointLight->position = *position;
    pointLight->color = *color;
    pointLight->intensity = intensity;
    pointLightRecalcMaxFactor(pointLight);
}

void pointLightSetColor(struct PointLight* pointLight, struct Coloru8* color) {
    pointLight->color = *color;
    pointLightRecalcMaxFactor(pointLight);
}
