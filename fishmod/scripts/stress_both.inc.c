// Both stress benches in one ROM (fishmod_debug_stress.inc.c, FISHMOD_STRESS 3): the fish bench in the Lakeside
// Laboratory's tank (as stress_fish.inc.c), then, warped to Lake Hylia, the plant bench (as stress_plants.inc.c:
// the deep basin's bare floor at noon, the scene's props off, a kelp forest 40 more plants a step). Three passes each;
// both results to SRAM after every step (fishmod/tools/stress_report.py reads both).
// Build: fishmod/build.sh --script stress_both --hw
#define FISHMOD_STRESS 3
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKESIDE_LABORATORY_0
#define FISHMOD_SCRIPT_ADULT 1

#define FISHMOD_SCRIPT_ROSTER 300
static Vec3f sScriptStartPos = { 0.0f, -360.0f, -60.0f };
static s16 sScriptStartYaw = -0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
