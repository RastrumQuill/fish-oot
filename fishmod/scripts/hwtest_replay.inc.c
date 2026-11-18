// The console test's replay (fishmod/pc/hwsuite.py): a recording attached to the ROM played from power-on with the
// frame log on and the sampler off; at its end a FISHMOD-CHECK line with the replay's counts (all zero: in step), then
// FISHMOD-DONE.
// Build: fishmod/build.sh --script hwtest_replay ...   then fishmod/tools/inputrec.py attach ROM REC OUT
#define FISHMOD_PCPROF_NOSAMPLE 1
#include "inputplay.inc.c"
