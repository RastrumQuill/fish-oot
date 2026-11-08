// The VR4300 timing tests (fishmod_debug_vrtime.inc.c, n64prof/vrtime/): Link stands in Kokiri Forest while they
// run, a second after the game starts. Fresh save (the results go over the save files). Read with
// n64prof/tools/vrtime.py save.sav.
// Build: fishmod/build.sh --script vrtime --hw
#define FISHMOD_VRTIME 1

static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
