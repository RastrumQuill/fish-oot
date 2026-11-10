// The profile with a log of every frame, over a SummerCart64's USB (fishmod_debug_pcprof.inc.c, FISHMOD_FRAMELOG): the
// play bench's route at Lake Hylia as pcprof.inc.c (every time of day and weather, the game as shipped on every pass),
// each frame's record (its period; the CPU's update and draw by section and by actor category; the RSP; the RDP and
// its counters; the costliest actors) and the profile's samples tagged by whether their frame ran over 52 ms. Without a
// SummerCart64, the plain profile in the save. Read with fishmod/tools/framelog.py and n64prof/tools/n64prof.py.
// Build: fishmod/build.sh --script pcprof_frames --hw   (the game as shipped: GCC, IDO for audio and libultra)
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define PB_BLOCKS 1 // each leg walked once (the first pass, as in play): about an hour for the eight laps
#include "hwbench_play.inc.c"
