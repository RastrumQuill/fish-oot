// The fish and plant stress benches (as stress_both.inc.c: the fish in the Lakeside Laboratory's tank 6 more a step,
// then a kelp forest in Lake Hylia's deep basin 40 more a step, each until the game can't hold 20 fps), with the
// profile and the frame log over a SummerCart64's USB (fishmod_debug_pcprof.inc.c) instead of only the save: every
// frame record tagged with its bench, step and pass (word 1: bench << 24 | step << 16 | pass << 8 | measured), each
// step's result as a FISHMOD-STRESS line, then FISHMOD-DONE. Two passes each (about 25 minutes in all).
// Read with fishmod/tools/stress_prof.py FOLDER --elf ELF.
// Build: fishmod/build.sh --script stress_prof --hw --colpack --gcc-all
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_NORING 1
#ifndef STRESS_PASSES
#define STRESS_PASSES 2
#endif
#include "stress_both.inc.c"
