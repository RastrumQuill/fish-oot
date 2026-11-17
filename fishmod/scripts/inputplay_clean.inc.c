// The replay for measuring from outside (ares's exact profile): no sampler, no frame log, no USB, and with
// fishmod/build.sh --no-timers none of the debug timers, so what runs is the game's code and the replay's few lines a
// frame. The counts are in gFishmodReplay (its `ended` word turns 1 after the recording's last frame).
// Build: fishmod/build.sh --script inputplay_clean --no-timers --hw ...
#define FISHMOD_INPUTPLAY 1
#define FISHMOD_NO_RESEED 1
#define FISHMOD_SCRIPT_NORMAL_BOOT 1

static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
