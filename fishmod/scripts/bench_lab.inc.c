// Benchmark (stress): adult Link in the Iron Boots at the bottom of the Lakeside Laboratory's deep tank, with the
// tank packed with fish at every depth.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKESIDE_LABORATORY_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_FISH_TANK 36
static Vec3f sScriptStartPos = { 0.0f, -360.0f, -60.0f };
static s16 sScriptStartYaw = -0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
