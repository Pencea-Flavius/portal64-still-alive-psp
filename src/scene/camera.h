#ifndef _CAMERA_H
#define _CAMERA_H

#include <ultra64.h>

#include "graphics/render_types.h"
#include "graphics/renderstate.h"
#include "math/boxs16.h"
#include "math/quaternion.h"
#include "math/rotated_box.h"
#include "math/vector3.h"
#include "math/transform.h"
#include "math/plane.h"
#include "physics/collision_quad.h"

#define CLIPPING_PLANE_LEFT         0
#define CLIPPING_PLANE_BOTTOM       1
#define CLIPPING_PLANE_RIGHT        2
#define CLIPPING_PLANE_TOP          3
#define CLIPPING_PLANE_NEAR         4
#define CLIPPING_PLANE_FAR          5
#define MAX_CLIPPING_PLANE_COUNT    6

struct Camera {
    struct Transform transform;
    float nearPlane;
    float farPlane;
    float fov;
};

struct FrustumCullingInformation {
    struct Plane clippingPlanes[MAX_CLIPPING_PLANE_COUNT];
    short usedClippingPlaneCount;

    struct Vector3 cameraPos;
};

struct CameraMatrixInfo {
    RenderMatrices projectionView;
    u16 perspectiveNormalize;
    struct FrustumCullingInformation cullingInformation;
};

enum FrustumResult {
    FrustumResultOutside,
    FrustumResultInside,
    FrustumResultBoth,
};

void frustumFromQuad(struct Vector3* cameraPos, struct CollisionQuad* quad, struct FrustumCullingInformation* out);
enum FrustumResult isOutsideFrustum(struct FrustumCullingInformation* frustum, struct BoundingBoxs16* boundingBox);
int isRotatedBoxOutsideFrustum(struct FrustumCullingInformation* frustum, struct RotatedBox* rotatedBox);
int isSphereOutsideFrustum(struct FrustumCullingInformation* frustum, struct Vector3* scaledCenter, float scaledRadius);
int isQuadOutsideFrustum(struct FrustumCullingInformation* frustum, struct CollisionQuad* quad);

void cameraInit(struct Camera* camera, float fov, float near, float far);
void cameraBuildViewMatrix(struct Camera* camera, float matrix[4][4]);
void cameraBuildProjectionMatrix(struct Camera* camera, float matrix[4][4], u16* perspectiveNorm, float aspectRatio);
float cameraClipDistance(struct Camera* camera, float distance);

// Both are plain float maths on a matrix the caller already has, and the half
// that hands the matrices to the renderer needs them.
void cameraExtractClippingPlane(float viewPersp[4][4], struct Plane* output, int axis, float direction);
int cameraIsValidMatrix(float matrix[4][4]);

int fogIntValue(float floatValue);

// Handing the matrices to the renderer is the platform's, and the build puts
// one half of it on the include path.
#include "camera_render.h"

#endif