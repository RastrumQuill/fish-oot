// The bed-skin test, looked at: views from a diver's height down onto the bed (on sand, on
// weed-bed green and on the edges between; picked by fishmod/tools/gen_bedskin.py --spots),
// each frozen and shown with the coloured bed and without it.
// Build: fishmod/build.sh --script look_bedskin --opt 5 --hw --bedskin; capture:
// fishmod/tools/edge_ab_capture.py ROM OUT --stops N ("new" has it, "old" is the plain bed).
#define FISHMOD_EDGE_AB 1
#define FISHMOD_TOUR_AUTO 400
#define FISHMOD_EDGE_AB_FLAGS FISHMOD_BENCH_NOBEDSKIN
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1        // a clear day
#define FISHMOD_SCRIPT_TIME 0x8000  // midday
#define FISHMOD_TOUR
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { -2070, -1794, 5560 }, { -1859, -1944, 5651 }, "1. sand", "", "", "" },
    { { 1092, -1691, 6299 }, { 1044, -1841, 6074 }, "2. sand", "", "", "" },
    { { -621, -1718, 4998 }, { -816, -1868, 5121 }, "3. sand", "", "", "" },
    { { -1223, -2251, 6245 }, { -1066, -2401, 6077 }, "4. green", "", "", "" },
    { { -1689, -1466, 7737 }, { -1477, -1616, 7825 }, "5. green", "", "", "" },
    { { -582, -1583, 8089 }, { -655, -1733, 8307 }, "6. green", "", "", "" },
    { { 643, -1479, 7839 }, { 741, -1629, 8047 }, "7. edge", "", "", "" },
    { { 1419, -1430, 6899 }, { 1635, -1580, 6819 }, "8. edge", "", "", "" },
    { { -3102, -1502, 5697 }, { -3044, -1652, 5474 }, "9. edge", "", "", "" },
};
