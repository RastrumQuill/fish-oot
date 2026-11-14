// The north-beach dock from five cameras (a tour that moves on by itself: frames to capture at each stop), for its
// design (fishmod/tools/gen_habitat.py, mesh_dock_*; fishmod/docs/dock-designs.png): from the beach, side on from the
// water, under the water, from across the lake, close on a post. Clear midday.
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

// The dock: its shore end at (-400, -1288, 3080), running out along +z for 420; the water at -1313.
static FishmodTourStop sTourStops[] = {
    { { -360, -1243, 2960 }, { -400, -1298, 3300 }, "Dock: from the beach", "", "", "" },
    { { -630, -1296, 3230 }, { -400, -1308, 3310 }, "Dock: side on", "", "", "" },
    { { -540, -1343, 3200 }, { -400, -1333, 3340 }, "Dock: under the water", "", "", "" },
    { { 100, -1248, 3780 }, { -400, -1298, 3280 }, "Dock: from across the lake", "", "", "" },
    { { -340, -1278, 3250 }, { -369, -1298, 3280 }, "Dock: a post, close", "", "", "" },
};
