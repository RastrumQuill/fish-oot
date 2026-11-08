// The fish stress bench (fishmod_debug_stress.inc.c, FISHMOD_STRESS 1): adult Link in the Iron Boots at the bottom of
// the Lakeside Laboratory's tank, Lake Hylia's fish let in 6 more a step until the game can't hold 20 fps. Three
// passes; the results to SRAM after every step (fishmod/tools/stress_report.py).
// Build: fishmod/build.sh --script stress_fish --hw
#define FISHMOD_STRESS 1
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKESIDE_LABORATORY_0
#define FISHMOD_SCRIPT_ADULT 1

#define FISHMOD_SCRIPT_ROSTER 300
static Vec3f sScriptStartPos = { 0.0f, -360.0f, -60.0f };
static s16 sScriptStartYaw = -0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
