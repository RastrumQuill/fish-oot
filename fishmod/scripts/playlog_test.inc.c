// Checks the play log in ares (playlog.inc.c is the one to play): starts at Lake Hylia and walks about for a while.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_PLAYLOG 1

static Vec3f sScriptStartPos = { -649.0f, -1250.0f, 3121.0f };
static s16 sScriptStartYaw = (s16)0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 300, 0, 0, 0, true }, { 400, 0, 0, 80, true }, { 200, 0, 60, 40, true },
    { 400, 0, 0, 80, true }, { 600, 0, -60, 60, true }, { 600, 0, 0, 0, true },
};
