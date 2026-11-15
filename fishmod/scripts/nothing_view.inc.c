// The anatomy sweep's "nothing" condition held still (fishmod_debug_bench.inc.c FISHMOD_ANATOMY_SWEEP, its last
// step): every part of the picture the sweep can leave out, left out, at the dock's first camera. On the console that
// still took 7.6 ms of RDP a frame (anatomy-sweep): its display list, walked with fishmod/tools/rdp_cost.py, says what.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_TOUR
#define FISHMOD_SCRIPT_BENCH                                                                                          \
    (FISHMOD_BENCH_NOROOMOPA | FISHMOD_BENCH_NOLINK | FISHMOD_BENCH_NOACTORS | FISHMOD_BENCH_NOEFFECTS |               \
     FISHMOD_BENCH_NOPROPS | FISHMOD_BENCH_NOFISH | FISHMOD_BENCH_NOROOMXLU | FISHMOD_BENCH_NOSKY | FISHMOD_BENCH_NOHUD)
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { -360, -1243, 2960 }, { -400, -1298, 3300 }, "Nothing drawn", "", "", "" },
};
