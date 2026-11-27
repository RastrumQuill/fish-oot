// The leap, tested: the auto-angler at the end of Lake Hylia's north-beach dock (deeper water than off the beach), a
// worm on every cast and a fish called to it at once
// (FISHMOD_ANGLER_CALL), every hooked fish holding at the bottom and leaping as often as it can, the rod never
// dropped for it (so hooks are thrown at the top). What happened is counted in gFishmodFT (FT_*, fishmod.h).
// fight_leap_far: the same with the rush's reach cut to a sixth (FIGHT_TEST_FAR), so the fish swim up for the leap
// first, as they do from water deeper than this.
// Build: fishmod/build.sh --script fight_leap --testmode
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ROSTER 300
#define FISHMOD_AUTO_ANGLER 1
#define FISHMOD_ANGLER_BAIT FISHMOD_BAIT_WORM
#define FISHMOD_ANGLER_CALL 1
#define FISHMOD_ANGLER_NO_DIP 1
#define FISHMOD_ANGLER_POWER 0.9f
#ifndef FISHMOD_ANGLER_FIGHT_TEST
#define FISHMOD_ANGLER_FIGHT_TEST (FIGHT_TEST_LEAP | FIGHT_TEST_DEEP)
#endif
static Vec3f sScriptStartPos = { -400.0f, -1240.0f, 3300.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0, true },
    { 18, 0, 0, 80, true }, // out toward the end of the dock
    { 30, 0, 0, 0, true },
};
