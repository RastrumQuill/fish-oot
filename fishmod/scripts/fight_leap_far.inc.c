// fight_leap with the rush's reach cut to a sixth: the swim up before the leap (see fight_leap.inc.c).
// Build: fishmod/build.sh --script fight_leap_far --testmode
#define FISHMOD_ANGLER_FIGHT_TEST (FIGHT_TEST_LEAP | FIGHT_TEST_DEEP | FIGHT_TEST_FAR)
#include "fight_leap.inc.c"
