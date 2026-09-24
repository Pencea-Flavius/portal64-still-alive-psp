#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <ultra64.h>

#include "vector4.h"
#include "vector3.h"

// Float 4x4 helpers (and MAX/MIN) formerly from libultra's <PR/gu.h>.
void matrixIdentity(float matrix[4][4]);

// out = a * b, in libultra's guMtxCatF order. Safe when out aliases either.
void matrixMul(float a[4][4], float b[4][4], float out[4][4]);

void matrixPerspective(float matrix[4][4], unsigned short* perspNorm, float l, float r, float top, float b, float near, float far);

// The same projection from a vertical field of view in degrees.
void matrixPerspectiveFov(float matrix[4][4], unsigned short* perspNorm, float fovDegrees, float aspectRatio, float near, float far);

float matrixNormalizedZValue(float depth, float nearPlane, float farPlane);

void matrixVec3Mul(float matrix[4][4], struct Vector3* input, struct Vector4* output);

void matrixFromBasis(float matrix[4][4], struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z);
#ifndef PSP
// Produces an RSP matrix, for the N64 renderer only.
void matrixFromBasisL(Mtx* matrix, struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z);
#endif

#endif
