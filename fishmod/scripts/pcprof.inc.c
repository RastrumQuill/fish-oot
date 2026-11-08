// The PC-sampling profile (fishmod_debug_pcprof.inc.c), for a compiler's sample profile: the play bench's route
// (Lake Hylia: walking, running, swimming, diving and fishing, every time of day and weather), with the game as
// shipped on every pass. Fresh save (it writes over the save files); the profile is saved every 30 seconds, so any
// length of run is readable. Read with fishmod/tools/pcprof.py save.sav.
// Build: fishmod/build.sh --script pcprof --hw
#define FISHMOD_PCPROF 1
#include "hwbench_play.inc.c"
