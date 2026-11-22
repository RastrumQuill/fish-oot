// The tackle at Lake Hylia's north beach: the auto-angler casting long with sinklure on the line (FishmodRig 2),
// for looking at it in the water and at the camera going under after it.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ROSTER 300
#define FISHMOD_AUTO_ANGLER 1
#define FISHMOD_ANGLER_RIG 2
#define FISHMOD_ANGLER_POWER 0.9f
static Vec3f sScriptStartPos = { -649.0f, -1283.0f, 3121.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },
};
