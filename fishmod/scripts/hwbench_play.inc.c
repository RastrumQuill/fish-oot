// The play bench at Lake Hylia (fishmod_debug.c, FISHMOD_BENCH_PLAY): adult Link (as the test build's warp: Zora
// tunic, the iron boots on every other leg) walks the bed, runs, swims, dives and fishes his way
// round the tour's stops with the game's own camera, each leg timed in turn as a first pass and then as shipped; the
// fish frozen; fish not drawn; no fish; no props; no water surface; none of these. One time of day and weather per
// lap (dawn, midday, dusk, night; clear and rain). Sums to SRAM after every leg, for fishmod/tools/hwbench_report.py.
// Build: fishmod/build.sh --script hwbench_play --opt 5 --hw --f3dex3
#define FISHMOD_HWBENCH 1
#define FISHMOD_BENCH_PLAY 1
#define FISHMOD_SCRIPT_ADULT 1 // as the test build warps him to the lake
#define FISHMOD_BENCH_LAPS 8 // one of each time and weather: about an hour a lap
#include "tour.inc.c"
