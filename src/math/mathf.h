
#ifndef _MATH_MATHF_H
#define _MATH_MATHF_H

// libultra spells these in <PR/gu.h>, so code that still includes <ultra64.h>
// already has them; the guard is what keeps that from clashing. Shared code
// that has been unhooked from the N64 headers gets them from here instead.
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// M_PI is the same story. <math.h> only defines it outside strict ISO C, which
// is what -std=c17 asks for, and libultra's headers supplied it on the N64.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int randomInt();
int randomInRange(int min, int maxPlusOne);
float randomInRangef(float min, float max);

float mathfLerp(float from, float to, float t);
float mathfInvLerp(float from, float to, float value);
float mathfMoveTowards(float from, float to, float maxMove);
float mathfBounceBackLerp(float t);
float mathfRandomFloat();
float mathfMod(float input, float divisor);
float clampf(float input, float min, float max);
float signf(float input);

int sign(int input);
int abs(int input);

float sqrtf(float in);
float powf(float base, float exp);

float cosf(float in);
float sinf(float in);
float fabsf(float in);
float floorf(float in);
float ceilf(float in);

float minf(float a, float b);
float maxf(float a, float b);

char floatTos8norm(float input);

float safeInvert(float input);

#endif