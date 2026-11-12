// The profile with a log of every frame, over a SummerCart64's USB (fishmod_debug_pcprof.inc.c, FISHMOD_FRAMELOG): the
// play bench's route at Lake Hylia as pcprof.inc.c (every time of day and weather, the game as shipped on every pass),
// each frame's record (its period; the CPU's update and draw by section and by actor category; the RSP; the RDP and
// its counters; the costliest actors) and the profile. (Its samples tagged by their frame, slow or not, crashed on the
// console in frames1, and not in ares: off, FISHMOD_PCPROF_NORING; bisect1.) Without a
// SummerCart64, the plain profile in the save. Read with fishmod/tools/framelog.py and n64prof/tools/n64prof.py.
// Build: fishmod/build.sh --script pcprof_frames --hw   (the game as shipped: GCC, IDO for audio and libultra)
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_CALIB 1 // (the calibration loops once at the start: fishmod/tools/inst_profile.py)
#ifndef FISHMOD_FRAME_TAGS
#define FISHMOD_PCPROF_NORING 1
#endif
#define PB_BLOCKS 1 // each leg walked once (the first pass, as in play)
#ifndef FISHMOD_BENCH_FULL
// the short bench (the default): one lap, clear dawn, of the legs that stutter (1-11 and 21), about 4 minutes, then
// FISHMOD-DONE; pcprof_frames_full has the whole route, two laps (clear dawn, rain at midday)
#define FISHMOD_BENCH_SHORT 1
#define FISHMOD_BENCH_LAPS 1
#else
#define FISHMOD_BENCH_LAPS 2
#endif
#include "hwbench_play.inc.c"
