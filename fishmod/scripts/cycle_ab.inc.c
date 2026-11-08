// An exact A/B of the props' 1-cycle drawing against the old 2-cycle at every tour stop (fishmod_debug.c,
// FISHMOD_EDGE_AB with another flag): the scene frozen once the camera settles, the modes taking turns every 60
// frames. Captured by fishmod/tools/edge_ab_capture.py.
// Build: fishmod/build.sh --script cycle_ab --hw
#define FISHMOD_EDGE_AB 1
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_TWOCYCLE
#define FISHMOD_TOUR_AUTO 400
#include "tour.inc.c"
