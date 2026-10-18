// One view per lake section, picked at random from section_budget.py's game-like cameras (the rod camera from the
// shore, swimming, diving), not by cost: for look_shots (captures), tour_shots (fly between them) and hwbench_shots
// (the console bench at the same cameras).
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
    { { -3491, -1165, 3074 }, { -3335, -1313, 3740 }, "1. Gerudo river inflow", "fish view, estimated 42.0 ms", "", "" },
    { { -2069, -1004, 3282 }, { -2597, -1313, 4179 }, "2. Lab shore and dock", "fish view, estimated 39.2 ms", "", "" },
    { { 896, -1331, 3540 }, { 744, -1351, 3410 }, "3. North beach shallows", "dive view, estimated 57.8 ms", "", "" },
    { { 2231, -1371, 5527 }, { 2369, -1391, 5382 }, "4. East shore lily shallows", "dive view, estimated 41.7 ms", "", "" },
    { { -3946, -1177, 4068 }, { -3118, -1313, 4789 }, "5. West bridge and grave is", "fish view, estimated 44.0 ms", "", "" },
    { { -3564, -1233, 5683 }, { -3784, -1283, 5683 }, "6. West weed flat", "swim view, estimated 74.9 ms", "", "" },
    { { -533, -1417, 8047 }, { -347, -1437, 7974 }, "7. Drop-off (weed edge)", "dive view, estimated 41.6 ms", "", "" },
    { { -1440, -1233, 4961 }, { -1249, -1283, 5070 }, "8. Deep basin", "swim view, estimated 26.6 ms", "", "" },
    { { -606, -1183, 7212 }, { -1221, -1313, 6221 }, "9. Water Temple gate (deepe", "fish view, estimated 36.1 ms", "", "" },
    { { -2106, -1233, 6775 }, { -1940, -1283, 6918 }, "10. South bridge pilings", "swim view, estimated 37.6 ms", "", "" },
    { { -933, -1183, 7177 }, { -906, -1313, 6643 }, "11. Island shallows", "fish view, estimated 34.8 ms", "", "" },
    { { 787, -1238, 7335 }, { 828, -1313, 6442 }, "12. East weed flats", "fish view, estimated 27.1 ms", "", "" },
    { { 406, -1233, 8708 }, { 301, -1283, 8514 }, "13. South reed bank and lily", "swim view, estimated 29.4 ms", "", "" },
    { { -978, -1380, 3724 }, { -1049, -1400, 3912 }, "14. Pillar ruins (by the Zor", "dive view, estimated 42.5 ms", "", "" },
    { { -459, -1339, 3686 }, { -365, -1359, 3509 }, "15. Dry dock (north beach)", "dive view, estimated 54.7 ms", "", "" },
    { { -384, -1413, 6805 }, { -233, -1433, 6937 }, "16. Under the island's dead ", "dive view, estimated 44.0 ms", "", "" },
};
