// The play bench's route at Lake Hylia for measuring from outside (ares's exact profile): the short lap (the legs
// that stutter, clear dawn, each walked once as in play) with no sampler and no frame log; with --no-timers, none of
// the debug timers either. ARES_CPUPROF_MARK=<sPbLeg> cuts the profile at every leg, so two builds are compared
// over the same legs (fishmod/docs/code-size.md).
// Build: fishmod/build.sh --script bench_clean --no-timers --hw ...
#define PB_BLOCKS 1
#define FISHMOD_BENCH_SHORT 1
#define FISHMOD_BENCH_LAPS 1
#include "hwbench_play.inc.c"
