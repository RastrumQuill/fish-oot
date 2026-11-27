// The console's walk through every scene (the morning queue): prof_scenes with each frame's record sent over USB
// (the frame log, its sampler off, so the times are the build's own) and FISHMOD-DONE at its end. What it's for: that
// no scene crashes or hangs on the console, and each scene's frame times there.
// Build: fishmod/build.sh --script hw_scenes --hw --texpack --colpack --meshpack --gcc-all
#define FISHMOD_PCPROF 1
#define FISHMOD_PCPROF_NOSAMPLE 1
#include "prof_scenes.inc.c"
