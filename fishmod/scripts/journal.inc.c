// Fishing journal test: start in Link's house with some catches recorded, read the records plaque on the wall
// and page through it.
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LINKS_HOUSE_0
#define FISHMOD_JOURNAL_TEST 1

// in front of the plaque (78, 38, 116, facing -0x71C7), facing it
static Vec3f sScriptStartPos = { 66.0f, 0.0f, 83.0f };
static s16 sScriptStartYaw = -0x71C7 + 0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 40, 0, 0, 0 },
    { 1, BTN_A, 0, 0 }, // read
    { 70, 0, 0, 0 },
    { 1, BTN_A, 0, 0 }, // journal summary
    { 70, 0, 0, 0 },
    { 1, BTN_A, 0, 0 }, // species
    { 70, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 70, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 70, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 70, 0, 0, 0 },
};
