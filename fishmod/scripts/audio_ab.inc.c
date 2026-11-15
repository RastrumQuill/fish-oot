// The audio A/B: the frame-log bench (pcprof_frames, unchanged) for 6 laps on one power-on, every lap clear dawn: the
// audio whole, without its RSP tasks (the driver still runs), without the driver's update either, and round again
// (fishmod_debug_bench.inc.c, FISHMOD_AUDIO_AB). What the audio costs the frame, and how much of that is the RSP's and
// how much the CPU's: compare the laps' frame periods and slow frames (framelog.py). Each lap's end also sends the
// audio thread's own time by part, as the console counted it (debug.log: FISHMOD-AUDIO lap, level, frames, updates,
// then microseconds: the whole update, the sequences, the notes, the command lists; and the notes in the lists).
// About 24 minutes.
// Build: fishmod/build.sh --script audio_ab --hw --texpack --colpack --meshpack --gcc-all
#define FISHMOD_AUDIO_AB 1
#define FISHMOD_BENCH_LAPS 6
#include "pcprof_frames.inc.c"
