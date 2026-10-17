// Exact A/B of the far tiers (half-size far textures, the far single cards, their dissolve at the cutoff) against
// the whole far cards on full textures out to a hard cutoff (fishmod/tools/edge_ab_capture.py: "new" is as shipped,
// "old" has all three switched off).
#define FISHMOD_EDGE_AB_FLAGS (FISHMOD_BENCH_NOIMP | FISHMOD_BENCH_NOFARTEX | FISHMOD_BENCH_NODISSOLVE)
#include "edge_ab.inc.c"
