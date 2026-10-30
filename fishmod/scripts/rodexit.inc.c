// The rod lets go of the bottles when it's put away (build with --testmode for the warp). Kokiri shore, bugs in both
// C bottles:
//  1. rod out (C-Left), put away (B: the sword), then the C-Right bugs: Link lets them go (old builds took them as
//     bait, or refused them, once the rod had been held);
//  2. rod out again, D-pad Down to Lake Hylia (the rod is gone with the scene), the C-Down bugs: let go there too;
//  3. back to Kokiri (D-pad Up).
// The screenshots show the bugs let go (the bottle icons go empty) and the rod's banner never says "Bait: Bug".
#define FISHMOD_SCRIPT_BUGS 1

static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_B, 0, 0, true },       // the sword: rod away
    { 60, 0, 0, 0, true },
    { 1, BTN_CRIGHT, 0, 0, true },  // bugs: let them go
    { 140, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0, true },   // rod out
    { 60, 0, 0, 0, true },
    { 1, BTN_DDOWN, 0, 0, true },   // to Lake Hylia
    { 400, 0, 0, 0, true },
    { 1, BTN_CDOWN, 0, 0, true },   // bugs: let them go
    { 140, 0, 0, 0, true },
    { 1, BTN_DUP, 0, 0, true },     // back to Kokiri
    { 300, 0, 0, 0, true },
};
