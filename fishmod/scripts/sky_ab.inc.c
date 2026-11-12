// An exact A/B of frames11's frame clears and sky (no colour clear under Lake Hylia's sky, the Z clear by the RSP, the
// sky in 1-cycle at blend 0) against the old way (FISHMOD_BENCH_OLDSKY) at every tour stop, the scene frozen, taking
// turns every 60 frames: the pictures must be the same. Captured by fishmod/tools/edge_ab_capture.py.
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_OLDSKY
#include "edge_ab.inc.c"
