// The audio sweep's scenes leg (fishmod/docs/audio-sweep.md): the audio check (audlog: the audio thread in lockstep
// with the game, every output buffer hashed) through every scene of the game by its first entrance, 150 frames
// each, Link running, rolling and swinging meanwhile. The scenes come from the entrance table at run time
// (fishmod_debug.c, FISHMOD_SWEEP_SCENES).
// The sound setting, child Link at night, and which share of the scenes a run plays are options in the ROM's header
// (fishmod_debug.c, FISHMOD_SWEEP_SCENES), set per run by the runner: one build plays them all.
// Build and run: fishmod/tools/audio_sweep.sh
#define FISHMOD_AUDLOG 1
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
