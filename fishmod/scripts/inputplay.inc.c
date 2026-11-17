// Replays a recorded playthrough (fishmod_debug_inputrec.inc.c): the recording attached to the ROM
// (fishmod/tools/inputrec.py attach), played from power-on with the profile and the frame log running, in ares under
// its profilers or on the console. See fishmod/docs/input-replay.md.
// Build: fishmod/build.sh --script inputplay --hw ...   then: fishmod/tools/inputrec.py attach ROM REC OUT
#define FISHMOD_INPUTPLAY 1
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_NORING 1
#define FISHMOD_NO_RESEED 1
#define FISHMOD_SCRIPT_NORMAL_BOOT 1

static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
