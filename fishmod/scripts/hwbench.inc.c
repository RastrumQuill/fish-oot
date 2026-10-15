// The hardware bench, for a night on the console (fishmod_debug.c, FISHMOD_HWBENCH): the tour's stops in turn, each
// timed in several modes (as shipped, the props' textures in 4 bits, cold caches, no props, no fish) with the RDP's
// own counters, then memory tests. Sums to SRAM after every stop (it carries on from them when started again), for
// fishmod/tools/hwbench_report.py. Build: fishmod/build.sh --script hwbench --opt 5 --hw
#define FISHMOD_HWBENCH 1
#include "tour.inc.c"
