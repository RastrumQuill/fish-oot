// Kelp turn and moss fit check: views 7 and 11 (kelp, each also 60 degrees round) and 2 and 15 (moss) of
// shots.inc.c, held still for edge_ab_capture.py --stops 6.
#define FISHMOD_EDGE_AB 1
#define FISHMOD_TOUR_AUTO 400
#define FISHMOD_EDGE_AB_FLAGS 0
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1        // a clear day
#define FISHMOD_SCRIPT_TIME 0x5000  // about 7:30 in the morning
#define FISHMOD_TOUR
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

// The water's surface is at y -1313 (the full lake).
static FishmodTourStop sTourStops[] = {
    { { -2267, -1333, 4244 }, { -2459, -1353, 4300 }, "2 moss", "", "", "" },
    { { -513, -1340, 3703 }, { -391, -1360, 3545 }, "15 moss", "", "", "" },
};
