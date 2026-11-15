// The lake bed's caustics from five cameras above the water (a tour that moves on by itself), the water's surface not
// drawn (FISHMOD_BENCH_NOROOMXLU: the bed seen clearly), for tools/meshpack.py's caustic_fade: from the north beach
// down the slope, into the deep pit by the lab, the shallows, the pit's edge, and the lake from high up. Clear midday.
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

// The water at -1313; the deep pit about (-900, -2300, 6200).
static FishmodTourStop sTourStops[] = {
    { { -1500, -1150, 3600 }, { -1100, -1500, 4600 }, "Caustics: down the slope", "", "", "" },
    { { -900, -1000, 5000 }, { -900, -2000, 6200 }, "Caustics: the deep pit", "", "", "" },
    { { -2000, -1200, 3500 }, { -2500, -1350, 4000 }, "Caustics: the shallows", "", "", "" },
    { { -300, -1100, 5000 }, { -1000, -1900, 6300 }, "Caustics: the pit's edge", "", "", "" },
    { { -1500, -700, 3000 }, { -900, -1800, 6000 }, "Caustics: the lake from high up", "", "", "" },
};
