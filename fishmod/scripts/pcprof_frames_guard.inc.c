// pcprof_frames with the code guard (fishmod_debug.c, FISHMOD_CODEGUARD): the game's code checksummed a block at a time,
// all of it every few seconds; if anything has changed it, the game stops with "code changed at ..." and the changed
// block goes over USB (FISHMOD-CODEGUARD in the debug log). For the console's code corruption (frames7, frames8b,
// rdpbench2; fourway1 before them).
// Build: fishmod/build.sh --script pcprof_frames_guard --hw --colpack --gcc-all
#define FISHMOD_CODEGUARD 1
#include "pcprof_frames.inc.c"
