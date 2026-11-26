// A picture of every scene (the scenes sweep, FISHMOD_SWEEP_SCENES: each scene by its first entrance, 150 frames
// each), Link standing where he arrives: for comparing two builds' geometry scene by scene (the seam pass, the strip
// pass). Read with a debugger at the same frame of each leg (gFishmodSweepLeg: the entrance + 1).
#define FISHMOD_AUDWORLD 1
#define FISHMOD_SWEEP_SCENES 1
#define FISHMOD_AUDWORLD_FRAMES 150
#define FISHMOD_SCRIPT_ENTRANCE ENTR_HYRULE_FIELD_0
#define FISHMOD_SCRIPT_ADULT 1
static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0 },
};
