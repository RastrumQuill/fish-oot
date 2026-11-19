// The fight's indicators, to look at: the auto-angler at Lake Hylia's north beach with a worm on every cast and a
// 300-fish roster, each fight drawn out (FISHMOD_ANGLER_SLOW_FIGHT: barely reeling for its first 15 seconds).
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ROSTER 300
#define FISHMOD_AUTO_ANGLER 1
#define FISHMOD_ANGLER_BAIT FISHMOD_BAIT_WORM
#define FISHMOD_ANGLER_SLOW_FIGHT 1
static Vec3f sScriptStartPos = { -649.0f, -1283.0f, 3121.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },
};
