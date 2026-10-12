// Benchmark: child Link on Lake Hylia's north beach facing the open lake, with the scene's roster as large as it
// goes (the game's fish, their homes and AI, as in play).
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#ifndef FISHMOD_SCRIPT_ROSTER
#define FISHMOD_SCRIPT_ROSTER 30
#endif
static Vec3f sScriptStartPos = { -649.0f, -1283.0f, 3121.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
