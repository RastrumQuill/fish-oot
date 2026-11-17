// Every scene, for the profilers (fishmod/tools/sysprof.py, hotcode.py): the audio sweep's scenes leg (sweep_scenes)
// without the audio check, so the sound runs on its own thread as in play. Every scene in the entrance table by its
// first entrance, 150 frames each, Link running, rolling and swinging; a scene's opening cutscene plays to its end.
// A path through every scene with no player needed; a recorded playthrough (inputrec) is the fuller one.
// Run under ares with ARES_CPUPROF, ARES_CPUPROF_OVL (and ARES_CPUPROF_MARK=<gFishmodSweepLeg> to cut by scene).
#define FISHMOD_AUDWORLD 1
#define FISHMOD_SWEEP_SCENES 1
#define FISHMOD_AUDWORLD_FRAMES 150
#define FISHMOD_SCRIPT_ENTRANCE ENTR_HYRULE_FIELD_0
#define FISHMOD_SCRIPT_ADULT 1
static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 30, 0, 0, 0 },      // settle
    { 30, 0, 0, 80 },     // run
    { 1, BTN_A, 0, 80 },  // roll
    { 20, 0, 0, 80 },
    { 1, BTN_B, 0, 0 },   // swing
    { 12, 0, 0, 0 },
    { 1, BTN_B, 0, 0 },
    { 12, 0, 0, 0 },
    { 25, 0, 80, 0 },     // run right
    { 1, BTN_A, 80, 0 },
    { 20, 0, -80, 0 },    // and left
    { 10, BTN_R, 0, 0 },  // shield up
    { 30, 0, 0, -80 },    // back
};
