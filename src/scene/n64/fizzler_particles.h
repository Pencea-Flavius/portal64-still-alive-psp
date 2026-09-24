#ifndef __SCENE_N64_FIZZLER_PARTICLES_H__
#define __SCENE_N64_FIZZLER_PARTICLES_H__

// The N64 half of the fizzler's particles (src/scene/psp/...): a quad per
// particle, drifting across the field, in the machine's vertex layout.

struct Fizzler;

// Builds the vertices and seeds every particle.
void fizzlerParticlesInit(struct Fizzler* fizzler);

// Moves every particle a step, respawning any past the edge.
void fizzlerParticlesUpdate(struct Fizzler* fizzler);

#endif
