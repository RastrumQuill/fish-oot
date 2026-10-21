// The dense beds before and after thinning: one random diving spot in each of six bed sections (section_budget.py
// --seed 11), looked at both ways. Build twice (FISHMOD_UNTHINNED=1 for before), capture with --stops 12.
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
    { { -3973, -1385, 5340 }, { -3831, -1405, 5198 }, "6. West weed flat 0", "", "", "" },
    { { -3689, -1385, 5056 }, { -3831, -1405, 5198 }, "6. West weed flat 180", "", "", "" },
    { { 766, -1360, 3192 }, { 867, -1380, 3020 }, "3. North beach shallows 0", "", "", "" },
    { { 968, -1360, 2848 }, { 867, -1380, 3020 }, "3. North beach shallows 180", "", "", "" },
    { { 1293, -1706, 6054 }, { 1438, -1726, 5916 }, "12. East weed flats 0", "", "", "" },
    { { 1583, -1706, 5778 }, { 1438, -1726, 5916 }, "12. East weed flats 180", "", "", "" },
    { { -416, -1343, 3608 }, { -299, -1363, 3446 }, "15. Dry dock (north beac 0", "", "", "" },
    { { -182, -1343, 3284 }, { -299, -1363, 3446 }, "15. Dry dock (north beac 180", "", "", "" },
    { { 2515, -1344, 5679 }, { 2449, -1364, 5868 }, "4. East shore lily shal 0", "", "", "" },
    { { 2383, -1344, 6057 }, { 2449, -1364, 5868 }, "4. East shore lily shal 180", "", "", "" },
    { { 350, -1331, 8487 }, { 328, -1351, 8288 }, "13. South reed bank and  0", "", "", "" },
    { { 306, -1331, 8089 }, { 328, -1351, 8288 }, "13. South reed bank and  180", "", "", "" },
};
