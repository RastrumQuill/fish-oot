// The drained Lake Hylia as adult (the Water Temple not beaten): its rain all over the lake from the start.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_DRAINED 1
static Vec3f sScriptStartPos = { -649.0f, -1283.0f, 3121.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },
};
