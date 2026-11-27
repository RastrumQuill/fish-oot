// The lure staying a lure, tested: the auto-angler on Lake Hylia's north-beach dock with the pond lure on, a fish
// called to it at once, every other bite let go by (missed), the fights short and the nibbles quick
// (FISHMOD_ANGLER_QUICK). gFishmodFT counts the fights begun on the lure (FT_FIGHTS_LURE) against the misses, the
// losses and the catches (gFishmodRodInfo.caught).
// Build: fishmod/build.sh --script fight_lure --testmode
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ROSTER 300
#define FISHMOD_AUTO_ANGLER 1
#define FISHMOD_ANGLER_RIG 1
#define FISHMOD_ANGLER_CALL 1
#define FISHMOD_ANGLER_MISS_EVERY 2
#define FISHMOD_ANGLER_QUICK 1
static Vec3f sScriptStartPos = { -400.0f, -1240.0f, 3300.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0, true },
    { 18, 0, 0, 80, true }, // out toward the end of the dock
    { 30, 0, 0, 0, true },
};
