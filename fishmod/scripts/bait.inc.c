// The bait loop at the Kokiri shore: a beehive in front of Link is broken with the sword, the chunk of comb is
// scooped up with an empty bottle (C-Down), the rod comes out (C-Left) and the larva bottle baits the hook. Then a
// pause (pause background check) and the auto-angler takes over.
#define FISHMOD_SCRIPT_TEST_HIVE 1
#define FISHMOD_AUTO_ANGLER 1

static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 30, 0, 0, 0, true },
    { 1, BTN_B, 0, 0 },          // sword: break the hive
    { 50, 0, 0, 0, true },
    { 1, BTN_CDOWN, 0, 0 },      // swing the empty bottle at the chunk
    { 120, 0, 0, 0, true },
    { 1, BTN_A, 0, 0 },          // close the message (a few presses: the first may only finish the typing)
    { 25, 0, 0, 0, true },
    { 1, BTN_A, 0, 0 },
    { 25, 0, 0, 0, true },
    { 1, BTN_A, 0, 0 },
    { 25, 0, 0, 0, true },
    { 1, BTN_A, 0, 0 },
    { 40, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0 },      // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_CDOWN, 0, 0 },      // larva bottle: bait the hook
    { 60, 0, 0, 0, true },
    { 40, BTN_START, 0, 0, 0, PAUSE_STATE_MAIN + 1 },  // pause
    { 120, 0, 0, 0, true },
    { 40, BTN_START, 0, 0, 0, PAUSE_STATE_OFF + 1 },
    { 40, 0, 0, 0, true },
};
