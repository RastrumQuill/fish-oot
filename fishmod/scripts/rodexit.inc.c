// The rod lets go of the buttons when it's put away (build with --testmode for the warp). Kokiri shore, bugs in the
// C-Right bottle:
//  1. rod out (C-Left), put away (C-Left again), then the bug bottle: Link lets the bugs go (they were taken as bait
//     once the rod had been held);
//  2. rod out again, D-pad Down to Lake Hylia (the rod is gone with the scene), the bottle again (now empty: Link
//     swings it), and back to Kokiri.
// Screenshots show the bugs on the ground after step 1 and the C-Right icon (empty bottle) after it.
#define FISHMOD_SCRIPT_BUGS 1

static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod away
    { 60, 0, 0, 0, true },
    { 1, BTN_CRIGHT, 0, 0, true },  // bug bottle: let them go
    { 120, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_DDOWN, 0, 0, true },   // to Lake Hylia
    { 400, 0, 0, 0, true },
    { 1, BTN_CRIGHT, 0, 0, true },  // the (now empty) bottle: a swing
    { 120, 0, 0, 0, true },
    { 1, BTN_DUP, 0, 0, true },     // back to Kokiri
    { 400, 0, 0, 0, true },
};
