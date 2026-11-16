// The pop-in test at every tour stop (fishmod_debug.c, FISHMOD_POP_AB): the scene frozen once the camera settles, then
// the same view drawn with the props' distances taken nearer (every kind by one set's travel, every kind by
// POP_AB_STEP, then each kind alone), so what differs between two pictures is what pops in or changes tier over that
// much travel and nothing else. Captured and scored by fishmod/tools/pop_ab_capture.py.
// Build: fishmod/build.sh --script pop_ab --hw --colpack --meshpack --gcc-all
#define FISHMOD_POP_AB 1
#include "tour.inc.c"
