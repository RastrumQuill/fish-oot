// The new item page in use (fishmod/docs/roadmap.md, mockup C): adult Link at Lake Hylia pauses, walks the cursor
// down to the lures (the name panel shows "Crankbait"), puts the crankbait on C-Down (its icon flies there and
// shows on the button), steps left to the rod ("Fishing Rod") and down to bottle 5, unpauses and uses the lure (a
// placeholder: an error beep), pauses again, then turns to the equipment page (Link's preview drawn as before).
// Build: fishmod/build.sh --script invtest --hw --f3dex3
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x6000
#define FISHMOD_SCRIPT_NOTEXT 1
static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f };
static s16 sScriptStartYaw = 0x8000;
#define STICK(x, y) { 3, 0, x, y }, { 25, 0, 0, 0 }
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 }, // pause (the debug save opens on the map page)
    { 50, 0, 0, 0 },
    { 1, BTN_Z, 0, 0 }, // to the item page
    { 50, 0, 0, 0 },
    STICK(-80, 0), // off the R tab: Din's Fire
    STICK(0, -80), // Farore's Wind
    STICK(0, -80), // Nayru's Love
    STICK(0, -80), // the child trade item
    STICK(-80, 0), // the adult trade item
    STICK(-80, 0), // lure 3
    STICK(-80, 0), // lure 2
    STICK(-80, 0), // lure 1: "Crankbait"
    { 60, 0, 0, 0 },
    { 1, BTN_CDOWN, 0, 0 }, // onto C-Down
    { 60, 0, 0, 0 },
    STICK(-80, 0), // the rod: "Fishing Rod"
    { 60, 0, 0, 0 },
    STICK(0, -80), // bottle 1
    STICK(80, 0),
    STICK(80, 0),
    STICK(80, 0),
    STICK(80, 0), // bottle 5
    { 60, 0, 0, 0 },
    { 1, BTN_START, 0, 0 }, // unpause
    { 60, 0, 0, 0, true },
    { 1, BTN_CDOWN, 0, 0 }, // use the lure: a placeholder beep
    { 60, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 }, // pause again: the item page, the crankbait framed for C-Down
    { 80, 0, 0, 0 },
    { 1, BTN_Z, 0, 0 }, // the equipment page
    { 2000, 0, 0, 0 },
};
#undef STICK
