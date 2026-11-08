// The plant stress bench (fishmod_debug_stress.inc.c, FISHMOD_STRESS 2): adult Link in the Iron Boots on the bare floor
// of Lake Hylia's deep basin, the scene's props off, and a kelp forest ahead of him, 40 more plants a step until the
// game can't hold 20 fps. Three passes; the results to SRAM after every step (fishmod/tools/stress_report.py).
// Build: fishmod/build.sh --script stress_plants --hw
#define FISHMOD_STRESS 2
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1

#define FISHMOD_SCRIPT_TIME 0x8000 // (noon: the light the same every pass)
static Vec3f sScriptStartPos = { -1100.0f, -2000.0f, 5450.0f };
static s16 sScriptStartYaw = 0x1500;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
