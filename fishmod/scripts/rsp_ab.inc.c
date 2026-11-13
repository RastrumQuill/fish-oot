// The all-day RSP test: the frame-log bench (pcprof_frames) lap after lap on one power-on, for as long as it's left
// running (up to 200 laps, about 13 hours), with the audio's RSP tasks off for every other block of 8 laps (a whole
// rotation of the times of day and weather each: fishmod_debug_bench.inc.c, FISHMOD_RSP_AB). What the audio's RSP work
// costs the frame: compare the laps with RSP audio time against those without (framelog.py, word 6).
// Build: fishmod/build.sh --script rsp_ab --hw --colpack --gcc-all
#define FISHMOD_RSP_AB 1
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_NORING 1
#define PB_BLOCKS 1
#define FISHMOD_BENCH_SHORT 1
#define FISHMOD_BENCH_LAPS 200
#include "hwbench_play.inc.c"
