// pcprof_frames over the whole route: every leg, two laps (clear dawn, then rain at midday), about 11 minutes. For
// checking a build before calling it done (and for catching rare crashes); the short bench is pcprof_frames.
// Build: fishmod/build.sh --script pcprof_frames_full --hw --colpack --gcc-all
#define FISHMOD_BENCH_FULL 1
#include "pcprof_frames.inc.c"
