// The console test's bench (fishmod/pc/hwsuite.py, fishmod/docs/hw-suite.md): the frame-log bench's short lap at Lake
// Hylia (pcprof_frames) with the sampler off, so the frame times are the build's own and two builds of it can be
// compared on the console. Each frame's record still goes over USB; at the end a FISHMOD-CHECK line (frames, the
// props' sets and their hash) and FISHMOD-DONE.
// Build: fishmod/build.sh --script hwtest_frames --hw --texpack --colpack --meshpack ...
#define FISHMOD_PCPROF_NOSAMPLE 1
#include "pcprof_frames.inc.c"
