// The anatomy sweep (about 3.3 hours on one power-on): where the RDP's frame goes beyond what the RDP and prop sweeps
// took apart. The frame-log bench (pcprof_frames, the reseeded route) for 50 laps of clear dawn, every other lap the
// game as shipped and between them one part of the frame left out (fishmod_debug_bench.inc.c, FISHMOD_ANATOMY_SWEEP):
// the room's terrain, Link, the other actors, the effects; then everything the sweeps can leave out at once (what
// remains is the frame's own cost: clears, setup). Read it with fishmod/tools/anatomy_sweep_report.py, which also
// gives the RDP's idle share (its time less its pipeline's busy time: waiting on the RSP or the CPU).
// Build: fishmod/build.sh --script anatomy_sweep --hw --colpack --meshpack --gcc-all
#define FISHMOD_ANATOMY_SWEEP 1
#define FISHMOD_BENCH_LAPS 50
#define FISHMOD_NO_PROFILE_SEND 1
#include "pcprof_frames.inc.c"
