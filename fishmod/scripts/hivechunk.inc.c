// Breaks a beehive in front of Link with the sword, then backs away and waits, to see the chunk of comb it drops.
#define FISHMOD_SCRIPT_TEST_HIVE 1
#define FISHMOD_VIEW 1
#define FISHMOD_VIEW_START 150
// a fixed camera off to the side, looking at the ground in front of Link where the chunk lands
static Vec3f sViewEye = { 900.0f, 40.0f, -20.0f };
static Vec3f sViewAt = { 925.0f, 2.0f, -95.0f };

static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 30, 0, 0, 0, true },
    { 1, BTN_B, 0, 0 },      // sword: break the hive
    { 30, 0, 0, 0, true },
    { 1, BTN_B, 0, 0 },      // again, in case the first swing missed
    { 40, 0, 0, 0, true },
    { 1, BTN_Z, 0, 0 },      // camera behind Link
    { 1, 0, 0, 0, true },
    { 900, 0, 0, 0, true },
};
