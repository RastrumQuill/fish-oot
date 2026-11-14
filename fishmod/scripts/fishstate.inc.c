// The fish behaviour check for update refactors: lakecast (Lake Hylia's north beach, a 300-fish roster, a cast that
// sinks to the bottom) held long enough to fill all 14 state fingerprints (one every 200 frames: fishmod_debug.c,
// StateHash_Update). Fish near the bait think in full, those farther out glide or follow their leaders, the farthest
// sleep: every path of EnFishmod_Update. Two builds whose fish should behave the same must give the same 14 hashes.
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
    { 3000, 0, 0, 0 },        // the bait sinks; the fish come and go
};
