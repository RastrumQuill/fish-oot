// A/B of the skybox under water (FISHMOD_NO_SKY): mode A as now (no skybox with the camera under water, the frame
// cleared to the fog colour), mode B the skybox drawn as vanilla. At the 16 diving views of shots.inc.c.
// Captured by fishmod/tools/edge_ab_capture.py --stops 16: the diff table says whether any pixel changes.
#define FISHMOD_EDGE_AB 1
#define FISHMOD_TOUR_AUTO 400
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_SKYUNDER
#include "shots.inc.c"
