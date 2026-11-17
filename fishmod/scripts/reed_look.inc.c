// The water fog from above against the camera-under-water fog: pairs of cameras at the same spot, 45 units over the
// surface and 45 under it (nearer, the camera keeps to the surface), looking at the same plant bed down the north beach's slope: across it, down at it, and
// nearly straight down; then from the shore by the dock. Over and under should look alike (fishmod.c
// Fishmod_WaterFogAmount: the same fog, over the length of the view's ray inside the water). A tour that moves on
// by itself, a stop every 200 frames. Clear midday.
// NOT RELIABLE YET: the tour's camera is pulled to the surface when near it (the over and under cameras came out at
// about -1312, both at the water line): it needs a camera that holds its place before it can compare anything.
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

static FishmodTourStop sTourStops[] = {
    { { -4200, -1230, 5550 }, { -4444, -1320, 5850 }, "Reeds: the far bank", "", "", "" },
    { { -1150, -1358, 4400 }, { -900, -1453, 5100 }, "Fog: across, under", "", "", "" },
    { { -1150, -1268, 4400 }, { -1000, -1413, 4750 }, "Fog: down, over", "", "", "" },
    { { -1150, -1358, 4400 }, { -1000, -1503, 4750 }, "Fog: down, under", "", "", "" },
    { { -1050, -1268, 4600 }, { -1020, -1413, 4680 }, "Fog: straight down, over", "", "", "" },
    { { -1050, -1358, 4600 }, { -1020, -1503, 4680 }, "Fog: straight down, under", "", "", "" },
    { { -360, -1243, 2960 }, { -400, -1298, 3300 }, "Fog: from the shore", "", "", "" },
};
