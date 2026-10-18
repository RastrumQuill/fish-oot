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
    { { -646, -1338, 8989 }, { -245, -1468, 9288 }, "Shot 1", "estimated 44.2 ms", "", "" },
    { { -1537, -1387, 4136 }, { -1300, -1429, 4576 }, "Shot 2", "estimated 45.8 ms", "", "" },
    { { 2636, -1349, 5834 }, { 3136, -1491, 5827 }, "Shot 3", "estimated 45.4 ms", "", "" },
    { { 1732, -1342, 7884 }, { 1932, -1401, 8342 }, "Shot 4", "estimated 44.0 ms", "", "" },
    { { -1895, -1338, 8681 }, { -1704, -1473, 9143 }, "Shot 5", "estimated 40.1 ms", "", "" },
    { { 391, -1340, 8671 }, { 460, -1426, 9166 }, "Shot 6", "estimated 42.4 ms", "", "" },
    { { -3990, -1401, 5129 }, { -4436, -1490, 4903 }, "Shot 7", "estimated 44.7 ms", "", "" },
    { { 1911, -1427, 5022 }, { 2307, -1578, 4716 }, "Shot 8", "estimated 45.2 ms", "", "" },
    { { -3384, -1400, 7266 }, { -3883, -1498, 7288 }, "Shot 9", "estimated 45.3 ms", "", "" },
    { { 2246, -1338, 6449 }, { 2742, -1408, 6510 }, "Shot 10", "estimated 43.5 ms", "", "" },
    { { -3956, -1349, 6449 }, { -3549, -1417, 6740 }, "Shot 11", "estimated 46.0 ms", "", "" },
    { { -2411, -1476, 8197 }, { -2007, -1606, 8491 }, "Shot 12", "estimated 44.7 ms", "", "" },
};
