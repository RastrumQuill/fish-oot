// The deep water seen from above (Fishmod_DepthShade: the bed and props darker with their depth), from five cameras
// above the surface (a tour that moves on by itself): the north beach out over the slope, the lab island's pit from
// the bridge, the middle of the lake from high up, the shallows by the dock, and the lake toward the pit. Clear midday.
// depth_tour_off: the same without it (FISHMOD_BENCH_NODEPTHSHADE).
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
    { { -1500, -1180, 3500 }, { -1000, -1450, 4700 }, "Depth: down the slope", "", "", "" },
    { { -600, -1100, 5300 }, { -900, -1700, 6200 }, "Depth: the pit by the lab", "", "", "" },
    { { -1500, -700, 3000 }, { -900, -1600, 6000 }, "Depth: the lake from high up", "", "", "" },
    { { -360, -1243, 2960 }, { -400, -1298, 3300 }, "Depth: the shallows by the dock", "", "", "" },
    { { 200, -1150, 4300 }, { -800, -1600, 6000 }, "Depth: across toward the pit", "", "", "" },
};
