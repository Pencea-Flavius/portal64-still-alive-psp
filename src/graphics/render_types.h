#ifndef __RENDER_TYPES_H__
#define __RENDER_TYPES_H__

// Handles whose layout differs between the machines, so shared structs can
// hold them.

#ifdef PSP

#include <pspgum.h>

#include "graphics/psp/psp_model.h"

// Geometry the asset pipeline produced.
typedef const struct PspModel* ModelHandle;

// Drawing retained for later: a display list on the N64, nothing on the PSP.
typedef void* RenderDisplayList;

// Vertices generated at runtime, in the machine's layout.
typedef void* RenderVertices;

// A block of transforms the renderer reads.
typedef void* RenderMatrices;

// One transform stored in a game structure (RenderMatrices is a block).
typedef ScePspFMatrix4 RenderMatrix;

// A render stage's screen rectangle and depth range.
typedef void* RenderViewport;

#else

#include <ultra64.h>

typedef Gfx* ModelHandle;
typedef Gfx* RenderDisplayList;
typedef Vtx* RenderVertices;
typedef Mtx* RenderMatrices;
typedef Mtx RenderMatrix;
typedef Vp* RenderViewport;

#endif

// The frame being drawn into; only the tag is shared.
struct GraphicsTask;

#endif
