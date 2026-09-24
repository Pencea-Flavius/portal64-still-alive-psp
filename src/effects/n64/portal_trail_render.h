#ifndef __EFFECTS_N64_PORTAL_TRAIL_RENDER_H__
#define __EFFECTS_N64_PORTAL_TRAIL_RENDER_H__

// The N64 half of the portal projectile's trail; see src/effects/psp/portal_trail_render.h.
// One short model drawn several times along a line, each turned 152 degrees
// further.

struct PortalTrail;
struct RenderState;
struct MaterialState;
struct Camera;

void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState, struct Camera* fromCamera, int portalIndex);

#endif
