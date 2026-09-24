#include "physics/debug_renderer.h"

// The PSP half of drawing the physics contacts; see src/physics/n64/debug_renderer.c.
// Empty for now; GU_LINES from the frame scratch would do it.
void contactSolverDebugDraw(struct ContactSolver* contactSolver, struct RenderState* renderState) {
    (void)contactSolver; (void)renderState;
}
