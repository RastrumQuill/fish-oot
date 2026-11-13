// The RSP test: the frame-log bench (pcprof_frames, unchanged) for 4 laps on one power-on, every lap clear dawn, the
// audio's RSP tasks on, off, on, off (fishmod_debug_bench.inc.c, FISHMOD_RSP_AB): what the audio's RSP work costs the
// frame. About 16 minutes. Compare the laps with RSP audio time against those without (framelog.py, word 6).
// Build: fishmod/build.sh --script rsp_ab --hw --colpack --gcc-all
#define FISHMOD_RSP_AB 1
#define FISHMOD_BENCH_LAPS 4
#include "pcprof_frames.inc.c"
