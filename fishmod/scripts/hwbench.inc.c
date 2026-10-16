// The hardware bench, for a night on the console (fishmod_debug.c, FISHMOD_HWBENCH): the tour's stops in turn, each
// timed in several modes (as shipped; the props with the old edge mode; far props cut to outlines as before; near
// ones only, far ones only; no props; no props and no skybox; no props and no water surface) with the RDP's own
// counters, then memory tests. Sums to SRAM after every stop (it carries on from them when started again), for
// fishmod/tools/hwbench_report.py. Build: fishmod/build.sh --script hwbench --opt 5 --hw
#define FISHMOD_HWBENCH 1
#include "tour.inc.c"
