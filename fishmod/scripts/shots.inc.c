// One view per lake section, a random diving view (camera 200 behind and 40 above Link) from
// section_budget.py's game-like cameras, not picked by cost: for look_shots (captures), tour_shots (fly between them) and hwbench_shots
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
    { { -3562, -1328, 4006 }, { -3438, -1343, 3849 }, "1. Gerudo river inflow", "dive view, estimated 41.0 ms", "", "" },
    { { -2267, -1333, 4244 }, { -2459, -1353, 4300 }, "2. Lab shore and dock", "dive view, estimated 44.1 ms", "", "" },
    { { 1172, -1400, 3391 }, { 1071, -1420, 3218 }, "3. North beach shallows", "dive view, estimated 59.7 ms", "", "" },
    { { 2521, -1333, 5425 }, { 2613, -1353, 5602 }, "4. East shore lily shallows", "dive view, estimated 53.5 ms", "", "" },
    { { -3043, -1421, 4532 }, { -2849, -1441, 4485 }, "5. West bridge and grave is", "dive view, estimated 44.7 ms", "", "" },
    { { -3566, -1395, 5298 }, { -3437, -1415, 5450 }, "6. West weed flat", "dive view, estimated 47.4 ms", "", "" },
    { { -1904, -1442, 7038 }, { -1705, -1462, 7051 }, "7. Drop-off (weed edge)", "dive view, estimated 45.3 ms", "", "" },
    { { -1395, -2071, 5914 }, { -1197, -2091, 5883 }, "8. Deep basin", "dive view, estimated 24.0 ms", "", "" },
    { { -1024, -1892, 6046 }, { -1146, -1912, 6204 }, "9. Water Temple gate (deepe", "dive view, estimated 24.6 ms", "", "" },
    { { -2481, -1573, 6728 }, { -2292, -1593, 6793 }, "10. South bridge pilings", "dive view, estimated 30.7 ms", "", "" },
    { { -820, -1346, 6614 }, { -1004, -1366, 6692 }, "11. Island shallows", "dive view, estimated 34.6 ms", "", "" },
    { { 1333, -1396, 7115 }, { 1533, -1416, 7119 }, "12. East weed flats", "dive view, estimated 63.3 ms", "", "" },
    { { -646, -1507, 8318 }, { -647, -1527, 8518 }, "13. South reed bank and lily", "dive view, estimated 31.1 ms", "", "" },
    { { -1164, -1378, 3753 }, { -1040, -1398, 3911 }, "14. Pillar ruins (by the Zor", "dive view, estimated 37.3 ms", "", "" },
    { { -513, -1340, 3703 }, { -391, -1360, 3545 }, "15. Dry dock (north beach)", "dive view, estimated 54.9 ms", "", "" },
    { { -104, -1364, 7794 }, { -214, -1384, 7627 }, "16. Under the island's dead ", "dive view, estimated 33.4 ms", "", "" },
};
