// Cast test: rod out, cast east into the Kokiri pond, wait, twitch, wait, reel in.
static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 20, 0, 0, 0 },          // settle
    { 1, BTN_CLEFT, 0, 0 },   // take out the rod
    { 30, 0, 0, 0 },
    { 1, BTN_CLEFT, 0, 0 },   // cast...
    { 18, 0, 0, 80 },         // ...far (stick up during the wind-up)
    { 300, 0, 0, 0 },         // watch the bobber
    { 2, 0, 60, 0 },          // twitch
    { 2, 0, 0, 0 },
    { 2, 0, -60, 0 },         // twitch
    { 400, 0, 0, 0 },
    { 200, BTN_A, 0, 0 },     // reel in
    { 60, 0, 0, 0 },
};
