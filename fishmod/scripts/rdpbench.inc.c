// The RDP calibration bench (fishmod_debug_rdpbench.inc.c, FISHMOD_RDP_BENCH): with the game's own drawing off, controlled
// drawing one test at a time (fill and texture rectangles by format, filter, cycle type and blending; triangle grids of
// four sizes in six modes; texture loads; syncs and mode switches), each held 30 frames, three times round (about five
// minutes). Every frame's record goes to the frame log over the SummerCart64's USB, tagged with its test; the console's
// RDP time and counters for each are what an RDP cost model is fitted to (fishmod/tools/rdpbench.py FOLDER).
// Build: fishmod/build.sh --script rdpbench --hw --gcc-all
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_NORING 1
#define FISHMOD_RDP_BENCH 1
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x6000
#define FISHMOD_SCRIPT_NOTEXT 1
static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f };
static s16 sScriptStartYaw = 0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 30000, 0, 0, 0, true },
};
