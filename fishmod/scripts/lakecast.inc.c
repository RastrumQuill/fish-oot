// Float depth test at Lake Hylia: from the north beach, cast out over the lake and let the bait sink until it
// stops by itself near the bottom (no A press), then wait, with a 300-fish roster around.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ROSTER 300
static Vec3f sScriptStartPos = { -649.0f, -1283.0f, 3121.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 90, 0, 0, 0, true },    // settle (gameplay frames: after the entrance)
    { 1, BTN_CLEFT, 0, 0 },   // take out the rod
    { 60, 0, 0, 0 },
    { 40, BTN_CLEFT, 0, 90 }, // hold to wind up, aiming far...
    { 1, 0, 0, 0 },           // ...and let go to cast
    { 1200, 0, 0, 0 },        // watch the bait sink and the fish
};
