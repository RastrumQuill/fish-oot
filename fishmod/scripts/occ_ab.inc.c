// An exact A/B of the occlusion plane at every tour stop (edge_ab's method): the scene frozen once the camera settles,
// the plane on and off taking turns every 60 frames. The two must match: the plane may only drop what's hidden.
// Captured by fishmod/tools/edge_ab_capture.py.
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_NOOCC
#include "edge_ab.inc.c"
