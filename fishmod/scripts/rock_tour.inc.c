// The lake's rocks, each made for where it lies (fishmod/tools/rock_mesh.py), close up from five cameras just above the
// water (a tour that moves on by itself), the water's surface not drawn (the bench's NOROOMXLU: no haze, the caustics
// clear): two outcrops (one in the shallows, one deeper, its caustics half faded), a stone, a boulder by the shore,
// and a rock deep down (no caustics).
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_TOUR
#define FISHMOD_TOUR_AUTO 200
#define FISHMOD_SCRIPT_BENCH FISHMOD_BENCH_NOROOMXLU
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { -2870, -1290, 4330 }, { -3012, -1470, 4483 }, "Rocks: an outcrop in the shallows", "", "", "" },
    { { -2660, -1290, 5190 }, { -2794, -1690, 5329 }, "Rocks: an outcrop deeper down", "", "", "" },
    { { -3520, -1300, 3800 }, { -3577, -1335, 3854 }, "Rocks: a stone", "", "", "" },
    { { -3390, -1120, 3360 }, { -3537, -1170, 3506 }, "Rocks: a boulder by the shore", "", "", "" },
    { { 1080, -1290, 5150 }, { 1005, -1850, 5232 }, "Rocks: deep down", "", "", "" },
};
