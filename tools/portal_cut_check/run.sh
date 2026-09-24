#!/bin/sh
# Runs the game's portal cutter on the host over every portal surface of every
# level, at many positions, rolls and opening scales, and fails if any cut
# would draw a triangle turned over or leave part of the wall out.
#
# Needs the PSP target built once (it reads the generated level geometry) and
# the container, for the PSP SDK headers the shared code includes:
#
#   docker run --rm -v ./:/usr/src/app portal64 tools/portal_cut_check/run.sh
#
# ONLY=<surface> checks one surface, STEPMIN=100 only full size portals,
# TRACE=1 prints every loop to stderr, and any argument prints every bad cut.
set -e
cd "$(dirname "$0")/../.."
OUT=build/portal_cut_check
mkdir -p $OUT
python3 tools/portal_cut_check/extract.py > $OUT/surfaces.h
gcc -O1 -g -DPSP -w \
    -I src -I src/scene -I src/effects/psp -I src/font/psp -I src/graphics/psp -I src/levels/psp \
    -I src/menu/psp -I src/physics/psp -I src/scene/psp -I src/sk64/psp -I src/system/psp/ultra64 -I build/psp \
    -I /usr/local/pspdev/psp/sdk/include -I $OUT \
    tools/portal_cut_check/harness.c tools/portal_cut_check/stubs.c \
    src/scene/portal_surface_generator.c src/scene/portal_surface.c src/math/*.c \
    -lm -o $OUT/harness
$OUT/harness "$@"
