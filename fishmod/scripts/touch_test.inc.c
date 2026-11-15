// Plants making way for Link (fishmod_scatter.c, Scatter_Touch): Link put in the water off the north beach, swims
// north through a patch of lily pads (and their flowers), then waits while they drift back. Frames to capture as he
// goes (framedump.sh: the pads pushed aside by his wake).
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
#define FISHMOD_SCRIPT_TIME 0x8000
static Vec3f sScriptStartPos = { -1324.0f, -1300.0f, 3580.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0, true }, // settle in the water
    { 120, 0, 0, 55 },     // swim north through the pads
    { 200, 0, 0, 0 },      // still: they drift back
};
