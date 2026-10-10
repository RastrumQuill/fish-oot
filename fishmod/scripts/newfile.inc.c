// Boots normally and makes a new file: title, file select, name entry, start. Then saves from the pause menu.
// Build with --testmode to check the test-mode new file; loadfile.inc.c loads the save again.
#define FISHMOD_SCRIPT_NORMAL_BOOT 1

static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 400, 0, 0, 0 },         // logo, title
    { 1, BTN_START, 0, 0 },   // title: to file select
    { 80, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // file 1 (empty): name entry
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // type a letter or few
    { 20, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 20, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 20, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },   // cursor to END (name entry reads buttons while drawing, so press it a few times)
    { 10, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },
    { 10, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },
    { 30, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // END
    { 300, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // file 1, then yes, open it (repeated: a single-frame press can be missed)
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 200, 0, 0, 0, true },   // into the game
    { 40, BTN_START, 0, 0, 0, PAUSE_STATE_MAIN + 1 },        // pause (as soon as the game lets it)
    { 20, 0, 0, 0, true },
    { 40, BTN_B, 0, 0, 0, PAUSE_STATE_SAVE_PROMPT + 1 },     // save prompt
    { 30, 0, 0, 0, true },
    { 1, BTN_A, 0, 0 },                                      // yes, save
    { 100, 0, 0, 0, true },
    { 40, BTN_START, 0, 0, 0, PAUSE_STATE_OFF + 1 },         // back to the game
    { 200, 0, 0, 0, true },
};
