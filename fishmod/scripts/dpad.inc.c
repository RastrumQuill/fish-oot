// The D-pad buttons in play: adult Link at Lake Hylia with Bombs on D-Up, the Iron Boots on D-Right and the Zora Tunic
// on D-Left (equipment marks; the HUD shows the cross and the three icons). Presses D-Right (Iron Boots on), D-Right
// again (back to the Kokiri Boots), D-Left (Zora Tunic, or back to Kokiri if he wore it), D-Left again, then D-Up
// (a bomb out), then pauses on the item page and puts the item under the cursor on D-Down.
// Build: fishmod/build.sh --script dpad
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0x6000
#define FISHMOD_SCRIPT_NOTEXT 1
#define FISHMOD_SCRIPT_DPAD                                                                                     \
    {                                                                                                           \
        SLOT_BOMB, SLOT_NONE, FISHMOD_EQUIP_SLOT(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA),                     \
            FISHMOD_EQUIP_SLOT(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON)                                        \
    }
static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f };
static s16 sScriptStartYaw = 0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 80, 0, 0, 0, true },
    { 1, BTN_DRIGHT, 0, 0 }, // Iron Boots
    { 80, 0, 0, 0, true },
    { 1, BTN_DRIGHT, 0, 0 }, // Kokiri Boots
    { 80, 0, 0, 0, true },
    { 1, BTN_DLEFT, 0, 0 }, // tunic
    { 80, 0, 0, 0, true },
    { 1, BTN_DLEFT, 0, 0 }, // and back
    { 80, 0, 0, 0, true },
    { 1, BTN_DUP, 0, 0 }, // a bomb
    { 120, 0, 0, 0, true },
    { 1, BTN_START, 0, 0 },
    { 50, 0, 0, 0 },
    { 1, BTN_Z, 0, 0 }, // the item page
    { 50, 0, 0, 0 },
    { 1, BTN_DDOWN, 0, 0 }, // what's under the cursor onto D-Down
    { 40, 0, 0, 0 },
    { 1, BTN_START, 0, 0 },
    { 100, 0, 0, 0, true },
};
