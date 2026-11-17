// The recorder's own test: a scripted start of the game recorded as a played one would be (a new file made at the file
// select, the opening cutscene with its text, out of Link's house into the forest and about), for the replay builds to
// play back. No controller needed: fishmod/docs/input-replay.md, "Testing".
#define INPUTREC_STEPS 1
#include "inputrec.inc.c"

#define MASH { 1, BTN_A, 0, 0 }, { 9, 0, 0, 0 },
#define MASH10 MASH MASH MASH MASH MASH MASH MASH MASH MASH MASH
#define MASH100 MASH10 MASH10 MASH10 MASH10 MASH10 MASH10 MASH10 MASH10 MASH10 MASH10

static Vec3f sScriptStartPos = { 0.0f, 0.0f, 0.0f };
static s16 sScriptStartYaw = 0;
static FishmodAutoplayStep sScriptSteps[] = {
    { 400, 0, 0, 0 },         // logo, title
    { 1, BTN_START, 0, 0 },   // title: to file select
    { 80, 0, 0, 0 },
    { 2, BTN_A, 0, 0 },       // file 1 (empty): name entry
    { 60, 0, 0, 0 },
    { 2, BTN_A, 0, 0 },       // type a few letters (two frames each: name entry reads the buttons while drawing)
    { 20, 0, 0, 0 },
    { 2, BTN_A, 0, 0 },
    { 20, 0, 0, 0 },
    { 2, BTN_A, 0, 0 },
    { 20, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },   // cursor to END
    { 10, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },
    { 10, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },
    { 30, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // END
    { 300, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },       // file 1, then yes, open it
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 60, 0, 0, 0 },
    { 1, BTN_A, 0, 0 },
    { 60, 0, 0, 0 },
    // the opening: the Deku Tree's and Navi's text (A pressed twice a second for 6 minutes)
    MASH100 MASH100 MASH100 MASH100 MASH100 MASH100 MASH100
    // out of the house and about the forest
    { 200, 0, 0, -80 },
    { 120, 0, 80, 0 },
    { 200, 0, 0, -80 },
    { 100, 0, -80, 0 },
    { 1, BTN_A, -80, 0 },
    { 100, 0, -80, -40 },
    { 200, 0, 0, 80 },
    { 100, 0, 0, 0 },
};
