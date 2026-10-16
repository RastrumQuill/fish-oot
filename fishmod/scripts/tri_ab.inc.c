// Exact A/B of the one-triangle cards and strict nearest-first order against the cut outlines and the old order
// (fishmod/tools/edge_ab_capture.py: "new" is as shipped, "old" has both switched back).
#define FISHMOD_EDGE_AB_FLAGS (FISHMOD_BENCH_NOTRI | FISHMOD_BENCH_NOSTRICT)
#include "edge_ab.inc.c"
