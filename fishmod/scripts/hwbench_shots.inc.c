// The hardware bench (hwbench.inc.c) at the picked shots (shots.inc.c) instead of the tour's stops: checks
// shot_search.py's frame-time estimates on the console. Build: fishmod/build.sh --script hwbench_shots --opt 5 --hw
// Read: hwbench_report.py save.sav (the stops are the shots, in order).
#define FISHMOD_HWBENCH 1
#define FISHMOD_BENCH_STOPS 12
#include "shots.inc.c"
