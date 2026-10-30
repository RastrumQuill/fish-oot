// Pause-menu reference shots for the item-page mockups (fishmod/tools/item_grid_mockups.py): adult Link on the Lake
// Hylia shore opens the pause menu, turns to the item page and waits there, with the debug save's full inventory.
// Build: fishmod/build.sh --script pausegrid   (pausegrid_empty: the same with the item page empty)
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x6000
#define FISHMOD_SCRIPT_NOTEXT 1
static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f };
static s16 sScriptStartYaw = 0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 60, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 }, // pause (the debug save opens on the map page)
    { 60, 0, 0, 0 },
    { 1, BTN_Z, 0, 0 },     // turn left to the item page
    { 60, 0, 0, 0 },
#ifndef FISHMOD_SCRIPT_EMPTY_ITEMS
    { 3, 0, -80, 0 },       // cursor off the R tab onto the grid (the cursor and an item name to compare with)
#endif
    { 2000, 0, 0, 0 },
};
