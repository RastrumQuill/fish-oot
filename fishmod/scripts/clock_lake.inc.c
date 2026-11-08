// The HUD clock at Lake Hylia near dusk (the sun low on the arc's right), with the D-pad set.
// Build: fishmod/build.sh --script clock_lake
#define FISHMOD_SCRIPT_ENTRANCE ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
#define FISHMOD_SCRIPT_TIME 0xC000
#define FISHMOD_SCRIPT_NOTEXT 1
#define FISHMOD_SCRIPT_DPAD { SLOT_BOMB, SLOT_NONE, FISHMOD_EQUIP_SLOT(EQUIP_TYPE_TUNIC, EQUIP_VALUE_TUNIC_ZORA), FISHMOD_EQUIP_SLOT(EQUIP_TYPE_BOOTS, EQUIP_VALUE_BOOTS_IRON) }
static Vec3f sScriptStartPos = { -2093.0f, -1037.0f, 705.0f };
static s16 sScriptStartYaw = 0x8000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 600, 0, 0, 0, true },
};
