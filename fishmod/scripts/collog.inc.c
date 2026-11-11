// The collision log: the play bench's route (as hwbench_play, each leg walked once, the game as shipped), every
// collision question and answer hashed a frame (z_bgcheck.c, FISHMOD_COLLOG). Two builds whose collision answers the
// same give the same hashes frame by frame: fishmod/tools/collog.sh runs one in ares and saves them,
// fishmod/tools/collog_compare.py compares two.
// Build: fishmod/build.sh --script collog --hw
#define FISHMOD_COLLOG 1
#define COLLOG_FRAMES 16000 // (about two laps)
#define PB_BLOCKS 1
#include "hwbench_play.inc.c"
