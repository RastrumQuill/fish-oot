// Collision test: Link dropped onto the middle of the north-beach dock (he should stand on the deck, y -1288,
// not fall into the water), facing out along it.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_DAY 1
static Vec3f sScriptStartPos = { -400.0f, -1240.0f, 3300.0f };
static s16 sScriptStartYaw = (s16)0x0000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0, true, 0 },
    // walk out to the end of the dock
    { 40, 0, 0, 80, true, 0 },
    { 200, 0, 0, 0, true, 0 },
};
