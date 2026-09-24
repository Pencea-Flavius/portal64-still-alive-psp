#ifndef __SCENE_PSP_FIZZLER_PARTICLES_H__
#define __SCENE_PSP_FIZZLER_PARTICLES_H__

// The PSP half of the fizzler's particles; see src/scene/n64/fizzler_particles.h.
// The fizzler builds its own quads, one per particle, in each machine's
// vertex layout.

struct Fizzler;

// Builds the geometry and seeds every particle. particleCount and the
// extents must be set.
void fizzlerParticlesInit(struct Fizzler* fizzler);

// Moves every particle one step and respawns the oldest once it is past the
// edge.
void fizzlerParticlesUpdate(struct Fizzler* fizzler);

#endif
