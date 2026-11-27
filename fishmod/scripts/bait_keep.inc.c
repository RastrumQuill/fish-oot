// The bait's keeping, tested: a worm on the hook, the pond lure on over it and off again, a worm bottle used with the
// lure on, and a scene change each way (Lake Hylia, Hyrule Field) with the bait on the hook and with it set aside.
// The readings are in gFishmodFT[FT_KEEP...] (fishmod_debug_angler.inc.c says which).
// Build: fishmod/build.sh --script bait_keep
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_AUTO_ANGLER 1
#define FISHMOD_ANGLER_BAITKEEP 1
static Vec3f sScriptStartPos = { -649.0f, -1283.0f, 3121.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },
};
