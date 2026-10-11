// Takes the rod out at the Kokiri shore, pauses once and waits (checks the pause menu's background picture with the
// rod out), then casts and pauses again with the line out.
static Vec3f sScriptStartPos = { 900.0f, 0.0f, -100.0f };
static s16 sScriptStartYaw = 0x2000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0 },     // rod out
    { 80, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 },     // pause (one press)
    { 300, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 },     // unpause
    { 80, 0, 0, 0, true },
    { 1, BTN_CLEFT, 0, 0 },     // cast
    { 200, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 },     // pause with the line out
    { 300, 0, 0, 0, true },
};
