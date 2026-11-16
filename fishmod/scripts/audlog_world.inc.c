// The audio check across the game: the audio check (audlog: the audio thread in lockstep with the game, every output
// buffer's samples hashed; fishmod/tools/collog.sh --audio, audlog_compare.py) on a tour of scenes instead of the
// lake's route. The driver isn't the same everywhere: the scenes use 11 audio session settings between them, with
// other reverbs (one downsampled, some filtered, some with a second tap), other voice counts and other music, and a
// change made exact at the lake can still differ elsewhere. Kokiri Forest first, then one or two scenes of each
// setting, 10 seconds each (fishmod_debug.c, FISHMOD_AUDWORLD), Link running, rolling and swinging his sword
// meanwhile; scenes that open with a cutscene play it. About 3 minutes of game.
// Build: fishmod/build.sh --script audlog_world --hw --texpack --colpack --meshpack --gcc-all
// Run:   fishmod/tools/collog.sh --audio ROM ELF 330 OUT.bin   (and audlog_compare.py --stream between builds whose
//        updates fall differently)
#define FISHMOD_AUDLOG 1
#define FISHMOD_AUDWORLD 1
#define FISHMOD_AUDWORLD_FRAMES 200
// (setting: 2 Hyrule Field; 1 the market, Kakariko; 5 Link's house; 0 Zora's Fountain; 3 the Shadow Temple, Jabu-Jabu;
// 4 the Fire Temple; 6 the Forest Temple, the Temple of Time; 8 the Desert Colossus, the Haunted Wasteland; 9 the
// Lost Woods, a Great Fairy's fountain; the Deku Tree; and the lake)
#define FISHMOD_AUDWORLD_ENTRANCES                                                                                   \
    ENTR_HYRULE_FIELD_0, ENTR_MARKET_DAY_0, ENTR_KAKARIKO_VILLAGE_0, ENTR_LINKS_HOUSE_0, ENTR_ZORAS_FOUNTAIN_0,       \
        ENTR_SHADOW_TEMPLE_0, ENTR_JABU_JABU_0, ENTR_FIRE_TEMPLE_0, ENTR_FOREST_TEMPLE_0, ENTR_TEMPLE_OF_TIME_0,     \
        ENTR_DESERT_COLOSSUS_0, ENTR_HAUNTED_WASTELAND_0, ENTR_LOST_WOODS_0, ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_0,     \
        ENTR_DEKU_TREE_0, ENTR_LAKE_HYLIA_0
#define FISHMOD_SCRIPT_ADULT 1
static Vec3f sScriptStartPos = { 1033.0f, 0.0f, 172.0f };
static s16 sScriptStartYaw = 0x4000;
static FishmodAutoplayStep sScriptSteps[] = {
    { 30, 0, 0, 0 },      // settle
    { 30, 0, 0, 80 },     // run
    { 1, BTN_A, 0, 80 },  // roll
    { 20, 0, 0, 80 },
    { 1, BTN_B, 0, 0 },   // swing
    { 12, 0, 0, 0 },
    { 1, BTN_B, 0, 0 },
    { 12, 0, 0, 0 },
    { 25, 0, 80, 0 },     // run right
    { 1, BTN_A, 80, 0 },
    { 20, 0, -80, 0 },    // and left
    { 10, BTN_R, 0, 0 },  // shield up
    { 30, 0, 0, -80 },    // back
};
