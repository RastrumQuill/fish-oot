// An exact A/B of the bed cards at every tour stop (edge_ab's method): the scene frozen once the camera settles, the
// cards on and off taking turns every 60 frames. Shows what the cards change on screen (they should look much the
// same as the plants they stand for). Captured by fishmod/tools/edge_ab_capture.py.
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_NOBEDCARD
#include "edge_ab.inc.c"
