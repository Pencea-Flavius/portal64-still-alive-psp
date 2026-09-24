# Checks the bounding sphere test in src/graphics/psp/psp_model_render.c
# (sphereAgainstClip) against the exact per-vertex outcodes it stands in for,
# on random matrices and parts: "inside" must mean no vertex is outside any
# clip plane, "outside" that every vertex is outside the same one. Mirrors
# the C formulas; update both together. Run: python3 tools/psp/sphere_clip_check.py
import math
import random
import sys

GUARD_BAND_X = 4.0
GUARD_BAND_Y = 7.0
PLANES = [(0, 0, 1, 1), (0, 0, -1, 1), (1, 0, 0, GUARD_BAND_X), (-1, 0, 0, GUARD_BAND_X), (0, 1, 0, GUARD_BAND_Y), (0, -1, 0, GUARD_BAND_Y)]

def clip(m, p):
    return [m[0][j] * p[0] + m[1][j] * p[1] + m[2][j] * p[2] + m[3][j] for j in range(4)]

def outcodes(c):
    return [sum(a[k] * c[k] for k in range(4)) < 0 for a in PLANES]

wrong = 0
random.seed(1)

for _ in range(20000):
    m = [[random.uniform(-2, 2) for _ in range(4)] for _ in range(4)]
    base = [random.uniform(-50, 50) for _ in range(3)]
    verts = [[base[i] + random.uniform(-20, 20) for i in range(3)] for _ in range(random.randint(3, 30))]
    lo = [min(v[i] for v in verts) for i in range(3)]
    hi = [max(v[i] for v in verts) for i in range(3)]
    center = [(lo[i] + hi[i]) / 2 for i in range(3)]
    radius = math.sqrt(max(sum((v[i] - center[i]) ** 2 for i in range(3)) for v in verts))

    c = clip(m, center)
    verdict = "inside"

    for a in PLANES:
        value = sum(a[k] * c[k] for k in range(4))
        gradient = [sum(a[k] * m[axis][k] for k in range(4)) for axis in range(3)]
        # reach squared: value +- reach against 0 without the square root.
        reachSquared = radius * radius * sum(g * g for g in gradient)

        if value < 0 and value * value > reachSquared:
            verdict = "outside"
            break

        if value < 0 or value * value < reachSquared:
            verdict = "crosses"

    codes = [outcodes(clip(m, v)) for v in verts]
    anyOutside = any(any(code) for code in codes)
    allOutsideOne = any(all(code[p] for code in codes) for p in range(len(PLANES)))

    if (verdict == "inside" and anyOutside) or (verdict == "outside" and not allOutsideOne):
        wrong += 1

print("wrong verdicts:", wrong)
sys.exit(1 if wrong else 0)
