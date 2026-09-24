#ifndef __EFFECTS_PSP_PORTAL_TRAIL_RENDER_H__
#define __EFFECTS_PSP_PORTAL_TRAIL_RENDER_H__

// The PSP half of the portal projectile's trail; see src/effects/n64/portal_trail_render.h.

struct PortalTrail;
struct RenderState;
struct MaterialState;
struct Camera;

void portalTrailRender(struct PortalTrail* trail, struct RenderState* renderState, struct MaterialState* materialState, struct Camera* fromCamera, int portalIndex);

#endif
