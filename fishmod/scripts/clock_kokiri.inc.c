// The HUD clock in Kokiri Forest at night (the moon on the arc's left).
// Build: fishmod/build.sh --script clock_kokiri
#define FISHMOD_SCRIPT_TIME 0x1800
#define FISHMOD_SCRIPT_NOTEXT 1
static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 600, 0, 0, 0, true },
};
