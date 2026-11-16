// The audio laps: the frame-log bench (pcprof_frames, unchanged) for 3 laps on one power-on, every lap clear dawn, the
// game as it is. Each lap's end sends the audio thread's own time by part, as the console counted it (debug.log:
// FISHMOD-AUDIO, as the audio A/B's: fishmod/scripts/audio_ab.inc.c). For comparing two builds of the audio, a run
// each: three laps of each to see past one lap's noise. About 12 minutes.
// Build: fishmod/build.sh --script audio_laps --hw --texpack --colpack --meshpack --gcc-all
//        (the two-refresh audio: the same with GCC_EXTRA_OPT=-DFISHMOD_AUDIO_EVERY2)
#define FISHMOD_AUDIO_LAPS 1
#define FISHMOD_BENCH_LAPS 3
#include "pcprof_frames.inc.c"
