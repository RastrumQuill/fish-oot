// The audio check: the play bench's route (as hwbench_play, each leg walked once, the game as shipped) with the audio
// thread in lockstep with the game (three updates a frame) and every output buffer's samples hashed
// (code/audio_thread_manager.c, FISHMOD_AUDLOG). Two builds whose audio code makes the same sound give the same hashes,
// buffer by buffer: fishmod/tools/collog.sh --audio saves the log, fishmod/tools/audlog_compare.py compares two.
// (Lockstep isn't real time: the sound itself plays wrong. Only the hashes count.)
// Build: fishmod/build.sh --script audlog --hw
#define FISHMOD_AUDLOG 1
#define PB_BLOCKS 1
#include "hwbench_play.inc.c"
