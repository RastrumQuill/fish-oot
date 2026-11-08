// The play bench with the four-way set (occlusion plane x bed cards, each on and off), and three references (no
// fish, no props, everything off), at every time of day and weather. Fresh save; read with
// fishmod/tools/hwbench_report.py save.sav (bench version 9).
// Build: fishmod/build.sh --script hwbench_fourway --hw
#define FISHMOD_BENCH_FOURWAY 1
#include "hwbench_play.inc.c"
