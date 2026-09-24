#ifndef __DEBUG_RENDERER_H__
#define __DEBUG_RENDERER_H__

#include "contact_solver.h"
#include "graphics/renderstate.h"

// Draws the physics contacts the solver is holding, which only the debug
// build asks for. The signature is the same on both machines and only the body
// differs, so there is no header pair: the two live in
// physics/{n64,psp}/debug_renderer.c.
void contactSolverDebugDraw(struct ContactSolver* contactSolver, struct RenderState* renderState);

#endif