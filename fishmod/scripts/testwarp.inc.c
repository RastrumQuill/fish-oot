// Test-mode warp check (build with --testmode too): in Kokiri Forest as child, L + D-pad Down to Lake Hylia (adult,
// full lake, Iron Boots), walk a little, then L + D-pad Up back to Kokiri Forest (child again).
static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0, true },
    { 1, BTN_L | BTN_DDOWN, 0, 0, true },  // to Lake Hylia
    { 400, 0, 0, 0, true },
    { 60, 0, 0, 60, true },        // walk
    { 300, 0, 0, 0, true },
    { 1, BTN_L | BTN_DUP, 0, 0, true },    // back to Kokiri Forest
    { 600, 0, 0, 0, true },
};
