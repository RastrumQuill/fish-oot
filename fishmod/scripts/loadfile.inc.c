// Boots normally and loads file 1 (for example the one newfile.inc.c saved: copy its .ram next to this ROM).
#define FISHMOD_SCRIPT_NORMAL_BOOT 1

static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 400, 0, 0, 0 },         // logo, title
    { 1, BTN_START, 0, 0 },   // title: to file select
    { 300, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // file 1, then load it (repeated: a single-frame press can be missed)
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 600, 0, 0, 0, true },   // play on the loaded file
};
