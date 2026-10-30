// The rod in a scene without fish: Zora's Domain (no spawn table), from the ledge over the big pool. Rod out, cast
// into the pool (the bobber floats: no fish, so nothing bites), then the empty C-Down bottle puts the rod away (the
// rod's controller goes with it), and the rod comes out and casts again.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_ZORAS_DOMAIN_0
static Vec3f sScriptStartPos = { -1112.0f, 210.0f, -160.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },    // settle
    { 1, BTN_CLEFT, 0, 0 },   // rod out
    { 60, 0, 0, 0 },
    { 40, BTN_CLEFT, 0, 90 }, // wind up...
    { 1, 0, 0, 0 },           // ...cast
    { 150, 0, 0, 0 },         // the bobber floats
    { 1, BTN_A, 0, 0 },       // stop the bait sinking (the reel waits for A to be let go)
    { 10, 0, 0, 0 },
    { 400, BTN_A, 0, 0 },     // reel it in
    { 30, 0, 0, 0 },
    { 1, BTN_CDOWN, 0, 0 },   // the bottle: rod away
    { 90, 0, 0, 0 },
    { 1, BTN_CLEFT, 0, 0 },   // rod out again
    { 60, 0, 0, 0 },
    { 40, BTN_CLEFT, 0, 90 },
    { 1, 0, 0, 0 },
    { 300, 0, 0, 0 },
};
