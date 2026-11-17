// Records a playthrough (fishmod_debug_inputrec.inc.c): the game from power-on, played on the controller, with every
// frame's inputs, the save memory and each scene's starting save context streamed over a SummerCart64's USB (the
// console, or the patched ares), alongside the PC-sampling profile and the frame log of the same play. Any later build
// replays it (inputplay.inc.c). See fishmod/docs/input-replay.md.
// Build: fishmod/build.sh --script inputrec --hw ...   Capture: `sc64deployer debug` running before power-on, its
// binaryout-*.bin files joined by fishmod/tools/inputrec.py join.
#define FISHMOD_INPUTREC 1
#define FISHMOD_PCPROF 1
#define FISHMOD_FRAMELOG 1
#define FISHMOD_PCPROF_NORING 1
#define FISHMOD_NO_RESEED 1 // (the random numbers run on as in play; the recording keeps their state)
#define FISHMOD_SCRIPT_NORMAL_BOOT 1

#ifndef INPUTREC_STEPS
static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 }, // (then the controller's own)
};
#endif
