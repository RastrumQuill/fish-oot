// Plants bending (fishmod_scatter.c): a point swept to and fro through a tall kelp a third of the way up (a rope: it
// gives way there, its top curving back up), the camera under the water beside it.
// Frames to capture through a sweep (framedump.sh): the weed bends away from the point as it passes, and springs back.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
#define FISHMOD_TOUR
#define FISHMOD_SCRIPT_BENCH FISHMOD_BENCH_NOROOMXLU
#define FISHMOD_TOUCH_TEST { -873.0f, -1558.0f, 8112.0f }
#define FISHMOD_TOUCH_SWEEP 30.0f
#define FISHMOD_TOUCH_PERIOD 160
static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};

static FishmodTourStop sTourStops[] = {
    { { -873, -1470, 7960 }, { -873, -1500, 8112 }, "Bending: kelp, low", "", "", "" },
};
