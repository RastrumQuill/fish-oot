// A bottle of worms baits the hook: worms in bottle 1 on C-Down, the rod out (C-Left), the hook baited from the bottle
// (one worm of three used), then the item page (the worm bottle's icon), for screenshots.
#define FISHMOD_SCRIPT_WORMS 1
#define FISHMOD_SCRIPT_NOTEXT 1
static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0 },      // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_CDOWN, 0, 0 },      // the worm bottle: bait the hook
    { 200, 0, 0, 0, true },
    { 40, BTN_START, 0, 0, 0, PAUSE_STATE_MAIN + 1 },  // pause
    { 30, 0, 0, 0, false },
    { 1, BTN_Z, 0, 0, false },                         // to the item page
    { 3000, 0, 0, 0, false },
};
