// Starts at Lake Hylia (where the road from Hyrule Field comes in) and stands there: checks the scene loads with
// fish and how the frame time compares with Kokiri.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0

static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 600, 0, 0, 0, true },
};
