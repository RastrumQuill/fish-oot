// A point swept to and fro through the spatterdock by the dock (FISHMOD_TOUCH_TEST, in Link's place, a swimmer's
// height): its leaves pushed aside, and back. The camera over the water.
// Frames to capture through a sweep (framedump.sh): the weed bends away from the point as it passes, and springs back.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_TOUR
#define FISHMOD_SCRIPT_BENCH FISHMOD_BENCH_NOROOMXLU
#define FISHMOD_TOUCH_TEST { -500.0f, -1335.0f, 3473.0f }
#define FISHMOD_TOUCH_SWEEP 60.0f
#define FISHMOD_TOUCH_PERIOD 160
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { -500, -1180, 3330 }, { -500, -1312, 3473 }, "Pushed: spatterdock", "", "", "" },
};
