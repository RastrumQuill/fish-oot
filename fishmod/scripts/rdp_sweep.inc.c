// The RDP sweep (about 9 hours on one power-on): the frame-log bench (pcprof_frames, the reseeded route: every lap
// the same scene) for 140 laps of clear dawn, every other lap the game as shipped and between them one thing left out
// in turn (fishmod_debug_bench.inc.c, FISHMOD_RDP_SWEEP): the props, the fish, the water surface, the sky, the HUD
// and minimap, then all of them. What each costs the RDP at each leg, against the laps beside it (the shipped laps
// show any drift over the night). The frame log only (the profile's samples not sent: about 70 MB).
// Build: fishmod/build.sh --script rdp_sweep --hw --colpack --meshpack --gcc-all
#define FISHMOD_RDP_SWEEP 1
#define FISHMOD_BENCH_LAPS 140
#define FISHMOD_NO_PROFILE_SEND 1
#include "pcprof_frames.inc.c"
