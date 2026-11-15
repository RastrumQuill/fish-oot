// Plants bending (fishmod_scatter.c, Scatter_Bend): a point swept to and fro across a ribbon weed off the north-east
// beach (861, -1367, 2873) (FISHMOD_TOUCH_TEST, in Link's place), the camera just above the water, its surface not drawn.
// Frames to capture through a sweep (framedump.sh): the weed bends away from the point as it passes, and springs back.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_TOUR
#define FISHMOD_SCRIPT_BENCH FISHMOD_BENCH_NOROOMXLU
#define FISHMOD_TOUCH_TEST { 861.0f, -1355.0f, 2873.0f }
#define FISHMOD_TOUCH_SWEEP 14.0f
#define FISHMOD_TOUCH_PERIOD 160
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { 861, -1338, 2805 }, { 861, -1352, 2873 }, "Bending: ribbon weed", "", "", "" },
};
