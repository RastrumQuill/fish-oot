// The terrain sweep (about 70 minutes on one power-on): two ways to draw the same picture for less, measured on the
// console. The frame-log bench (pcprof_frames, the reseeded route) for 18 laps of clear dawn, every other lap the game
// as shipped and between them (fishmod_debug_bench.inc.c, FISHMOD_TERRAIN_SWEEP): the terrain anti-aliased without
// reading the picture back (now the game's: FISHMOD_BENCH_TERRAIN_AA puts the read back; the coverage still written, the VI smooths the edges), the sky
// drawn last, only where nothing opaque is (FISHMOD_BENCH_SKYLAST: not painted under the terrain first), and both.
// ares: the two only change pixels along the polygons' edges (dock_tour_skylast). Read it with
// fishmod/tools/anatomy_sweep_report.py RUN_FOLDER --terrain.
// Build: fishmod/build.sh --script terrain_sweep --hw --colpack --meshpack --gcc-all
#define FISHMOD_TERRAIN_SWEEP 1
#define FISHMOD_BENCH_LAPS 18
#define FISHMOD_NO_PROFILE_SEND 1
#include "pcprof_frames.inc.c"
