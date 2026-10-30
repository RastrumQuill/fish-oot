// The rod lets go of the bottles when it's put away (build with --testmode for the warp). Kokiri shore; the rod on
// C-Left, the slingshot on C-Down, bugs in the C-Right bottle. (With the rod out, B and the rod's button cast and A
// has no "put away": switching to another item is how it's put away.)
//  1. rod out, the slingshot (the rod goes away), then the bugs: they mustn't go on the hook (old builds took them as
//     bait once the rod had been held). With the slingshot in hand the game doesn't pour them out either (vanilla),
//     so the bottle stays full and the rod, taken out again, says "No bait";
//  2. L + D-pad Down to Lake Hylia with the rod out (the rod goes with the scene), then the bugs: Link lets them go (the
//     C-Right bottle goes empty);
//  3. back to Kokiri (L + D-pad Up).
#define FISHMOD_SCRIPT_BUGS 1

static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_CDOWN, 0, 0, true },   // the slingshot: rod away
    { 60, 0, 0, 0, true },
    { 1, BTN_CRIGHT, 0, 0, true },  // bugs: let them go
    { 140, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_L | BTN_DDOWN, 0, 0, true },   // to Lake Hylia
    { 300, 0, 0, 0, true },
    { 1, BTN_CRIGHT, 0, 0, true },  // bugs: let them go
    { 100, 0, 0, 0, true },
    { 1, BTN_L | BTN_DUP, 0, 0, true },     // back to Kokiri
    { 300, 0, 0, 0, true },
};
