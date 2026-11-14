// The prop sweep (about 5.5 hours on one power-on): the frame-log bench (pcprof_frames, the reseeded route) for 84 laps
// of clear dawn, every other lap the game as shipped and between them one plant kind drawn another way
// (fishmod_debug_bench.inc.c, FISHMOD_PROP_SWEEP): left out (what it costs in all), its whole cards up close, or its cut
// outline split into the triangles of least height (gen_habitat.py low_triangulate). 21 tests, each twice. What each
// saves at each leg, against the shipped laps beside it: fishmod/tools/prop_sweep_report.py. The frame log only.
// Build: fishmod/build.sh --script prop_sweep --hw --colpack --meshpack --gcc-all
#define FISHMOD_PROP_SWEEP 1
#define FISHMOD_BENCH_LAPS 84
#define FISHMOD_NO_PROFILE_SEND 1
#include "pcprof_frames.inc.c"
