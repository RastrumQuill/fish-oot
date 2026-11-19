// The shallow bed close up, where the caustics fade with depth (tools/meshpack.py, caustic_fade): cameras just over
// the slope off the north beach and by the dock, looking down it. Clear midday. With FISHMOD_BENCH_NOROOMXLU,
// _NOWATERFOG and _NOFOG poked in, the bed as it is.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_TOUR
#define FISHMOD_TOUR_AUTO 200
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { -1300, -1200, 3700 }, { -1150, -1520, 4300 }, "Caustics: the slope, close", "", "", "" },
    { { -900, -1150, 3900 }, { -700, -1600, 4800 }, "Caustics: down the slope", "", "", "" },
    { { -400, -1180, 3000 }, { -300, -1400, 3600 }, "Caustics: by the dock", "", "", "" },
    { { -1700, -1000, 4200 }, { -1300, -1550, 4800 }, "Caustics: the slope from above", "", "", "" },
    { { 300, -1150, 3600 }, { 0, -1550, 4300 }, "Caustics: the east shallows", "", "", "" },
};
