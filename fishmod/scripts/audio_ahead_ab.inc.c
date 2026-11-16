// The build-ahead A/B: the audio laps (audio_laps: the frame-log bench, every lap clear dawn, each lap's end sending
// the audio thread's own time) for 4 laps on one power-on, the audio's updates built ahead in laps 1 and 3 (the update
// that takes the game's commands also makes the next two refreshes' lists and tasks, while its code is in the caches:
// audio/internal/thread.c) and one a refresh, as always, in laps 2 and 4. The same updates either way, and the same
// sound (the audio check). About 16 minutes. Needs the Expansion Pak (the build's --expak).
// Build: fishmod/build.sh --script audio_ahead_ab --hw --texpack --colpack --meshpack --gcc-all --expak
#define FISHMOD_AUDIO_AHEAD_AB 1
#define FISHMOD_AUDIO_LAPS 1
#define FISHMOD_BENCH_LAPS 4
#include "pcprof_frames.inc.c"
