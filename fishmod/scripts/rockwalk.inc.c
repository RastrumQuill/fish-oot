// Collision check for the reworked rocks: adult Link in the iron boots walks up onto the lake's biggest rock (by the
// west bridge, about 420 wide and 150 tall) and over it, his position on screen. He should climb its side, stand on
// its top (not float over it, not sink into it) and walk off the far side.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x8000
static Vec3f sScriptStartPos = { -3560.0f, -1450.0f, 4957.0f };
static s16 sScriptStartYaw = 0x4000; // facing +x, toward the rock
#define FISHMOD_SCRIPT_WALK_TO { -2880.0f, 0.0f, 4957.0f } // straight across its middle (it stands at x -3240)
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};
