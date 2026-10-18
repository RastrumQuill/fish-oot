// Under-water shots picked for looks within the 20 fps budget (rdp_estimate screen-span model, estimated RDP time
// 47 ms or less with a 30 ms no-prop baseline), each held still and captured by fishmod/tools/edge_ab_capture.py.
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
    { { 2587, -1361, 5856 }, { 2944, -1474, 6205 }, "Shot 1", "estimated 45.3 ms", "", "" },
    { { 1412, -1368, 8016 }, { 1443, -1509, 8515 }, "Shot 2", "estimated 43.9 ms", "", "" },
    { { -1592, -1338, 4123 }, { -1478, -1495, 3636 }, "Shot 3", "estimated 45.8 ms", "", "" },
    { { -646, -1467, 8769 }, { -433, -1543, 9222 }, "Shot 4", "estimated 40.0 ms", "", "" },
    { { -3658, -1402, 6897 }, { -3937, -1511, 7312 }, "Shot 5", "estimated 45.7 ms", "", "" },
    { { -1601, -1365, 8715 }, { -2071, -1474, 8888 }, "Shot 6", "estimated 45.8 ms", "", "" },
    { { 77, -1378, 8686 }, { -179, -1419, 9115 }, "Shot 7", "estimated 46.7 ms", "", "" },
    { { 893, -1379, 3194 }, { 1005, -1461, 3681 }, "Shot 8", "estimated 46.6 ms", "", "" },
    { { -2446, -1338, 4173 }, { -2220, -1380, 4619 }, "Shot 9", "estimated 46.1 ms", "", "" },
    { { -3508, -1338, 7616 }, { -3660, -1430, 8093 }, "Shot 10", "estimated 46.4 ms", "", "" },
    { { -3498, -1402, 4900 }, { -3751, -1532, 4468 }, "Shot 11", "estimated 46.8 ms", "", "" },
    { { -4238, -1338, 5844 }, { -4686, -1445, 5620 }, "Shot 12", "estimated 44.0 ms", "", "" },
};
