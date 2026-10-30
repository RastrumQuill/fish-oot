// The roaming hardware bench at Lake Hylia (fishmod_debug.c, FISHMOD_BENCH_ROAM): the camera flies from each tour
// stop to the next, looking about as it goes, and each leg is flown once per mode: as shipped; the fish frozen (no AI
// or swimming); the fish not drawn; no fish; no props; no water surface; no sky; none of these. Sums to SRAM after
// every leg, for fishmod/tools/hwbench_report.py. Build: fishmod/build.sh --script hwbench_roam --hw --f3dex3
#define FISHMOD_HWBENCH 1
#define FISHMOD_BENCH_ROAM 1
#define FISHMOD_BENCH_LAPS 12 // about 40 minutes a lap
#include "tour.inc.c"
