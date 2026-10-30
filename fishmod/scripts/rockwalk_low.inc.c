// Collision check for a low rock (only its top collides): adult Link in the iron boots walks over one 13 units tall
// at (-2182, 4700). He should step up onto it (his height rising by about that much), not float, not sink into it.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x8000
static Vec3f sScriptStartPos = { -2302.0f, -1550.0f, 4700.0f };
static s16 sScriptStartYaw = 0x4000;
#define FISHMOD_SCRIPT_WALK_TO { -2052.0f, 0.0f, 4700.0f }
#define FISHMOD_SCRIPT_WALK_PROBE { -2182.0f, 0.0f, 4700.0f } // the rock (top -1582, bed -1596)
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0, true },
};
