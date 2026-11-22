// The item page with the mod's icons on it (the tackle, a larva bottle): bait.inc.c's way to a larva bottle at the
// Kokiri shore (a hive broken, its comb scooped up, the rod out and baited once: the bottle at 2/3), then paused on the
// item page and left there, for screenshots.
#define FISHMOD_SCRIPT_TEST_HIVE 1
#define FISHMOD_SCRIPT_NOTEXT 1

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
    { 30, 0, 0, 0, false },
    { 1, BTN_Z, 0, 0, false },                         // (it opens on the map: one page left to the items)
    { 3000, 0, 0, 0, false },
};
