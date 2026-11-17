// The audio scheduling A/B: the audio laps (audio_laps: the frame-log bench, every lap clear dawn, each lap's end
// sending the audio thread's own time) for 8 laps on one power-on, four ways twice round:
//   laps 1, 5: as always (an update a refresh; its RSP task sent at the next refresh, taking the RSP there and then)
//   laps 2, 6: the updates built ahead (the update that takes the game's commands also makes the next two refreshes'
//              lists and tasks, while its code is in the caches: audio/internal/thread.c)
//   laps 3, 7: the RSP tasks sent with time to spare (each as soon as it's built, run when the RSP is next free, made
//              urgent only at the refresh the game would have sent it: code/audio_thread_manager.c, sched.c)
//   laps 4, 8: both (three tasks ready at once, each for a gap in the graphics' use of the RSP)
// The same updates and the same tasks in the same order every way: the same sound. About 32 minutes. Needs the
// Expansion Pak (the build's --expak).
// Build: fishmod/build.sh --script audio_sched_ab --hw --texpack --colpack --meshpack --gcc-all --expak
#define FISHMOD_AUDIO_SCHED_AB 1
#define FISHMOD_AUDIO_LAPS 1
#define FISHMOD_BENCH_LAPS 8
#include "pcprof_frames.inc.c"
