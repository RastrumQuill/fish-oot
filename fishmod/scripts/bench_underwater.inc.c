// Benchmark (stress): adult Link in the Iron Boots on the bottom of Lake Hylia, with 30 extra fish around him.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_EXTRA_FISH 30
static Vec3f sScriptStartPos = { -3585.0f, -1250.0f, 2690.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
