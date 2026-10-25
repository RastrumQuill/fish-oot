// The play log: the test build as played (title, file select, a new or saved file), recording at Lake Hylia where and
// why frames run slow (fishmod_debug.c, FISHMOD_PLAYLOG). The log goes to the end of SRAM after the save slots.
// Build: fishmod/build.sh --script playlog --opt 5 --hw --testmode [--f3dex3]
// Read: fishmod/tools/playlog_report.py save.sav
#define FISHMOD_SCRIPT_NORMAL_BOOT 1
#define FISHMOD_PLAYLOG 1

static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 1, 0, 0, 0 },
};
