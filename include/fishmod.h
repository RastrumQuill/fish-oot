#ifndef FISHMOD_H
#define FISHMOD_H

#include "ultra64.h"
#include "libu64/pad.h"
#include "z_math.h"

struct GameState;
struct PlayState;
struct GfxPrint;

#ifndef FISHMOD_DEBUG
#define FISHMOD_DEBUG 0
#endif

#include "fishmod_species_ids.h"

/* Species data (generated from fishmod/data/species.json, see fishmod_species.c) */

typedef enum FishmodBaseModel {
    /* 0 */ FISHMOD_BASE_BASS,
    /* 1 */ FISHMOD_BASE_LOACH,
    /* 2 */ FISHMOD_BASE_JABU,
    /* 3 */ FISHMOD_BASE_SMALL // the generic fish from gameplay_keep (the one you can put in a bottle)
} FishmodBaseModel;

typedef enum FishmodDepth {
    /* 0 */ FISHMOD_DEPTH_SURFACE,
    /* 1 */ FISHMOD_DEPTH_MID,
    /* 2 */ FISHMOD_DEPTH_BOTTOM
} FishmodDepth;

// Bit index in FishmodSpecies.activeTimes
typedef enum FishmodTimeOfDay {
    /* 0 */ FISHMOD_TIME_DAWN,
    /* 1 */ FISHMOD_TIME_DAY,
    /* 2 */ FISHMOD_TIME_DUSK,
    /* 3 */ FISHMOD_TIME_NIGHT
} FishmodTimeOfDay;

typedef enum FishmodBait {
    /* 0 */ FISHMOD_BAIT_NONE,
    /* 1 */ FISHMOD_BAIT_BEE_LARVA,
    /* 2 */ FISHMOD_BAIT_BUG,
    /* 3 */ FISHMOD_BAIT_WORM,
    /* 4 */ FISHMOD_BAIT_CORAL_EARRING,
    /* 5 */ FISHMOD_BAIT_FISH,
    // lures (how they're fished comes later)
    /* 6 */ FISHMOD_BAIT_LURE_SPINNER,
    /* 7 */ FISHMOD_BAIT_LURE_CRANKBAIT,
    /* 8 */ FISHMOD_BAIT_LURE_POPPER,
    /* 9 */ FISHMOD_BAIT_LURE_FROG,
    /* 10 */ FISHMOD_BAIT_LURE_SINKING,
    /* 11 */ FISHMOD_BAIT_MAX
} FishmodBait;

typedef struct FishmodBehavior {
    /* 0x00 */ u8 curiosity;  // how far away it notices the bobber and how readily it comes over
    /* 0x01 */ u8 aggression; // chance to commit to a bite once it is inspecting
    /* 0x02 */ u8 wariness;   // how easily splashes, Link and reeling spook it
    /* 0x03 */ u8 strength;   // how hard it pulls when hooked
    /* 0x04 */ u8 stamina;    // how long it can keep pulling
    /* 0x05 */ u8 speed;      // swim speed
    /* 0x06 */ u8 jumpiness;  // chance to leap when hooked
    /* 0x07 */ u8 nibbles;    // nibbles before the real bite
    /* 0x08 */ u8 biteWindow; // frames the bobber stays under for the hookset
} FishmodBehavior; // size = 0x09

typedef enum FishmodPart {
    /* 0 */ FISHMOD_PART_HEAD,
    /* 1 */ FISHMOD_PART_BODY_FRONT,
    /* 2 */ FISHMOD_PART_BODY_REAR,
    /* 3 */ FISHMOD_PART_TAIL,
    /* 4 */ FISHMOD_PART_FIN_DORSAL,
    /* 5 */ FISHMOD_PART_FIN_PECTORAL,
    /* 6 */ FISHMOD_PART_FIN_BELLY,
    /* 7 */ FISHMOD_PART_MAX,
    /* 0xFF */ FISHMOD_PART_NONE = 0xFF // joints without geometry: not tinted or scaled
} FishmodPart;

#define FISHMOD_LOOK_WHISKERS (1 << 0)

typedef struct FishmodSpecies {
    /* 0x00 */ const char* name;
    /* 0x04 */ u8 base;        // FishmodBaseModel
    /* 0x05 */ u8 depth;       // FishmodDepth
    /* 0x06 */ u8 rarity;      // 1-5
    /* 0x08 */ u16 price;      // rupees for a typical-size fish
    /* 0x0A */ u8 activeTimes; // bitmask of 1 << FishmodTimeOfDay
    /* 0x0B */ u8 shape[3];    // model scale percent: length, height, width
    /* 0x0E */ u8 tint[3];     // RGB multiplied into the model
    /* 0x11 */ u8 lengthMin;   // inches
    /* 0x12 */ u8 lengthTypical;
    /* 0x13 */ u8 lengthMax;
    /* 0x14 */ u16 weightCoef; // weight (lbs) = weightCoef * length^3 / 1000000
    /* 0x16 */ FishmodBehavior behavior;
    /* 0x1F */ u8 bait[FISHMOD_BAIT_MAX]; // interest multiplier percent per bait
    /* 0x2A */ u8 partTint[FISHMOD_PART_MAX][3];  // RGB per part
    /* 0x3F */ u8 partScale[FISHMOD_PART_MAX][3]; // percent per part: length, height, width
    /* 0x54 */ u8 lookFlags;                       // FISHMOD_LOOK_*
    /* 0x55 */ u8 activity;    // 0-3 per FishmodTimeOfDay, 2 bits each (dawn in the low bits)
    /* 0x56 */ u8 depthTop;    // the band of the water column it keeps to: 0 = surface, 255 = floor
    /* 0x57 */ u8 depthBottom;
    /* 0x58 */ u8 rise;        // how far up the band moves when it's active and the light is low (feeding)
    /* 0x59 */ u8 lightShy;    // how much bright light pushes it deeper
    /* 0x5A */ u8 chronotype;  // each fish's clock is shifted by up to this many hours
    /* 0x5B */ u8 schoolMin;   // lives in schools of this many to schoolMax (0: on its own)
    /* 0x5C */ u8 schoolMax;
} FishmodSpecies;

#define FISHMOD_ACTIVITY(species, tod) (((species)->activity >> (2 * (tod))) & 3)

typedef struct FishmodSpawnEntry {
    /* 0x00 */ u8 species;
    /* 0x01 */ u8 weight;
} FishmodSpawnEntry; // size = 0x02

typedef struct FishmodSceneSpawns {
    /* 0x00 */ s16 sceneId;
    /* 0x02 */ s16 count;
    /* 0x04 */ const FishmodSpawnEntry* entries;
    /* 0x08 */ s16 fish; // how many fish live there (0: FISHMOD_FISH_COUNT); the most is FISHMOD_ROSTER_MAX
} FishmodSceneSpawns; // size = 0x0C

extern const FishmodSpecies gFishmodSpecies[FISHMOD_SPECIES_MAX];
extern const FishmodSceneSpawns gFishmodSceneSpawns[];
extern const s32 gFishmodSceneSpawnsCount;

/* Core hooks (fishmod.c) */

typedef enum FishmodActorType {
    /* 0 */ FISHMOD_TYPE_ROD, // one per scene: rod, line, bobber, camera, fish spawning
    /* 1 */ FISHMOD_TYPE_FISH,
    /* 2 */ FISHMOD_TYPE_COMB, // a chunk of a broken beehive with larvae in it, scooped up with a bottle
    /* 3 */ FISHMOD_TYPE_MANAGER // one per scene (FISHMOD_OPT 4): the fish roster, drawing all the fish
} FishmodActorType;

#define FISHMOD_PARAMS(type, species) (((type) << 8) | (species))
#define FISHMOD_GET_TYPE(params) (((params) >> 8) & 3)
// a fish from the scene's roster (FISHMOD_OPT 4): its record index, in bits 10-14
#define FISHMOD_PARAMS_ROSTER(species, index) (FISHMOD_PARAMS(FISHMOD_TYPE_FISH, species) | (((index) + 1) << 10))
#define FISHMOD_GET_ROSTER(params) ((((params) >> 10) & 0x1F) - 1)
#define FISHMOD_GET_SPECIES(params) ((params) & 0xFF)

const FishmodSceneSpawns* Fishmod_GetSceneSpawns(s16 sceneId);
// Which of a scene's roster fish were kept today (FISHMOD_OPT 4): they stay gone until the day changes.
s32 Fishmod_RosterIsCaught(s16 sceneId, s32 day, s32 index);
void Fishmod_RosterSetCaught(s16 sceneId, s32 day, s32 index);
void Fishmod_OnSceneLoad(struct PlayState* play);
void Fishmod_PlayUpdate(struct PlayState* play);
s32 Fishmod_CanUseRod(struct PlayState* play);

extern struct Actor* gFishmodRod;

// Photo mode (debug renders for the wiki): fish hold still at their home position on a plain backdrop.
typedef struct FishmodPhoto {
    /* 0x00 */ u8 active;
    /* 0x01 */ u8 marker; // which photo is on screen, shown as a colored square for the capture script
    /* 0x02 */ s16 yaw;
    /* 0x04 */ s16 pitch;
    /* 0x08 */ Vec3f center; // set by the fish while drawing: middle of its limbs
    /* 0x14 */ f32 extent;   // and the distance between its farthest limbs
    /* 0x18 */ u32 backdropFrame; // frame the backdrop was last drawn, so only the first fish draws it
} FishmodPhoto; // size = 0x1C

extern FishmodPhoto gFishmodPhoto;

// Animation studio (debug, fishmod/scripts/anim.inc.c): fish posed in a row on the photo backdrop, moved by a
// script so their swimming can be recorded (fishmod/tools/anim_capture.sh). Used with gFishmodPhoto.active.
#define FISHMOD_STUDIO_MAX 4
typedef struct FishmodStudio {
    /* 0x00 */ u8 active;
    /* 0x01 */ s8 startle;      // one-shot: a startled flick this frame (sign: which way)
    /* 0x02 */ s16 yaw;
    /* 0x04 */ s16 pitch;
    /* 0x06 */ s16 roll;
    /* 0x08 */ s16 jawRot;
    /* 0x0C */ f32 swimMag;
    /* 0x10 */ f32 swimPhaseStep;
    /* 0x14 */ f32 spawnLength;  // size for the next fish spawned (inches), 0 = typical
    /* 0x18 */ struct Actor* fish[FISHMOD_STUDIO_MAX];
    /* 0x28 */ u8 rig[FISHMOD_STUDIO_MAX]; // per fish: 0 = as the game picks, 1 = full rig, 2 = reduced rig
    /* 0x2C */ u8 spawnLook;  // looks for the next fish spawned (FISHMOD_LOOK_NEUTRAL for the photos)
} FishmodStudio;

extern FishmodStudio gFishmodStudio;

// Rod status published every frame by the fishing controller (for the debug auto-angler and the HUD)
typedef struct FishmodRodInfo {
    /* 0x00 */ u8 state;  // FishmodRodState
    /* 0x01 */ u8 biting;
    /* 0x02 */ u8 nibbling;
    /* 0x04 */ f32 tension;
    /* 0x08 */ u16 caught; // fish landed this session
    /* 0x0A */ u16 lost;
    /* 0x0C */ f32 castPower;
    /* 0x10 */ u8 choice; // after a catch: 1 keep or release, 2 release only
    /* 0x14 */ const char* notice; // a one-line notice for the rod's banner, shown once
    /* 0x18 */ u8 bait;          // FishmodBait on the hook
    /* 0x19 */ u8 baitRequest;   // set by a bait bottle: put this on the hook
} FishmodRodInfo;

extern FishmodRodInfo gFishmodRodInfo;

/*
 * Saved fishing data. The save file has scene flags for 124 scenes but the game only has SCENE_ID_MAX (110),
 * so the last 14 entries (392 bytes) are never used by anything; fishmod keeps its data there. Old saves
 * read as all zeros, which means "nothing caught yet".
 */
#define FISHMOD_SAVE_SPECIES_MAX 32

typedef struct FishmodSpeciesRecord {
    /* 0x00 */ u16 count;        // fish of this species landed
    /* 0x02 */ u16 bestTenths;   // longest, in tenths of an inch
} FishmodSpeciesRecord; // size = 0x4

// What's in a bottle with a fish in it (the item itself only says "fish")
typedef struct FishmodBottleFish {
    /* 0x00 */ u8 species;      // species + 1; 0 for a fish bottled some other way (a vanilla fish)
    /* 0x01 */ u8 look;         // its own looks (colour and pattern), so it's the same fish when it's let go
    /* 0x02 */ u16 lengthTenths;
} FishmodBottleFish; // size = 0x4

#define FISHMOD_BAIT_SLOTS 8 // the save's (unused) bait pouch; not tied to FISHMOD_BAIT_MAX

typedef struct FishmodSaveData {
    /* 0x00 */ u32 flags;
    /* 0x04 */ FishmodSpeciesRecord records[FISHMOD_SAVE_SPECIES_MAX];
    /* 0x84 */ u8 baitCount[FISHMOD_BAIT_SLOTS];  // the bait pouch
    /* 0x8C */ FishmodBottleFish bottleFish[4];    // per bottle slot
} FishmodSaveData; // size = 0x9C

#define FISHMOD_SAVE ((FishmodSaveData*)&gSaveContext.save.info.sceneFlags[SCENE_ID_MAX])

// Bait bottles: new bottle contents in item IDs the game never uses. Using one from a C button while holding the
// rod baits the hook with one of what's in it.
#define ITEM_BOTTLE_LARVA_3 ITEM_INVALID_1 // three bee larvae
#define ITEM_BOTTLE_LARVA_2 ITEM_INVALID_2
#define ITEM_BOTTLE_LARVA_1 ITEM_INVALID_3
#define FISHMOD_IS_BAIT_ITEM(item) (((item) >= ITEM_BOTTLE_LARVA_3) && ((item) <= ITEM_BOTTLE_LARVA_1))
extern u64 gFishmodBottleLarva3Tex[];
extern u64 gFishmodBottleLarva2Tex[];
extern u64 gFishmodBottleLarva1Tex[];
const char* Fishmod_BaitName(s32 bait);
// Player is about to use `item` from a button: returns true if fishmod used it (baiting the hook)
s32 Fishmod_UseItem(struct PlayState* play, s32 item);
// The icon texture (in RAM) for fishmod's items, or NULL for a vanilla item
void* Fishmod_ItemIcon(s32 item);
// The item whose name the pause menu shows for `item`
s32 Fishmod_ItemNameIndex(s32 item);
// A bottle just caught `actorId` into inventory slot `slot` (fish records, the larva message)
void Fishmod_OnBottleCatch(s32 actorId, s32 slot);

// Bottled fish: what went into a bottle, and letting it go again (returns true if fishmod handled the release)
void Fishmod_BottleFish(s32 species, f32 lengthInches, s32 look);
s32 Fishmod_ReleaseBottleFish(struct PlayState* play, s32 slot, Vec3f* pos, s16 yaw);
// A length for the next fish spawned (0 for a random one), used by Fishmod_ReleaseBottleFish
extern f32 gFishmodSpawnLength;
extern u8 gFishmodSpawnLook; // with gFishmodSpawnLength: the released fish's looks
extern Vec3f gFishmodHivePos;

/** Records a catch. Returns true if it is a new size record for its species. */
s32 Fishmod_RecordCatch(s32 species, f32 lengthInches);
// Adds the fishing journal to the records plaque in Link's house (called by the message system)
void Fishmod_ExtendMessage(u16 textId, u8* buf, u16* length, s32 size);
#if FISHMOD_TESTMODE
// A new file in a test-mode build skips the intro and starts with bottles, a slingshot, the Goron's Bracelet etc.
void Fishmod_InitTestSave(void);
// A small frames-per-second readout in the top right corner
void Fishmod_DrawFps(Gfx** gfxP);
void Fishmod_TestPlayInit(void);
#endif
// The catch textbox: the reused vanilla text id it shows under, and the call that opens it
#define FISHMOD_CATCH_TEXT_ID 0x4083
void Fishmod_ShowCatchMessage(struct PlayState* play, const char* name, const char* size, s32 newRecord,
                              s32 canKeep);

#if FISHMOD_DEBUG
// CPU time spent per frame, for the debug timing line
typedef enum FishmodCpuTimer {
    FISHMOD_CPU_UPDATE, // all of Play_Update
    FISHMOD_CPU_DRAW,   // all of Play_Draw (building display lists)
    FISHMOD_CPU_FISH_UPDATE,
    FISHMOD_CPU_FISH_DRAW,
    FISHMOD_CPU_ROD_UPDATE,
    FISHMOD_CPU_ROD_DRAW,
    FISHMOD_CPU_MAX
} FishmodCpuTimer;
extern OSTime gFishmodCpuTime[FISHMOD_CPU_MAX];

// Finer timers for the parts of a fish's update and draw (fishmod/tools/read_stats.py --prof). Nested slots are
// also counted in the slot around them.
typedef enum FishmodProfSlot {
    FISHMOD_PROF_CALIBRATE,  // an empty timed block: what one timer costs
    FISHMOD_PROF_U_ENGINE,   // the whole actor->update call for fish (includes everything below)
    FISHMOD_PROF_U_GLIDE,    // the cheap in-between frames of throttled fish
    FISHMOD_PROF_U_AI,       // EnFishmod_FishAI: deciding and moving
    FISHMOD_PROF_U_WATER,    //   water surface/floor queries (inside AI)
    FISHMOD_PROF_U_RAYCAST,  //     collision raycasts for the floor (inside water queries)
    FISHMOD_PROF_U_SWIM,     // the swim wave (body and fin angles)
    FISHMOD_PROF_U_DEBUG,    // the debug readout block (not in release builds)
    FISHMOD_PROF_D_ENGINE,   // the whole Actor_Draw for fish: lights, root matrix, then the draw function
    FISHMOD_PROF_D_SETUP,    //   render setup and texture segments
    FISHMOD_PROF_D_ROOT,     //   the fish's root matrix
    FISHMOD_PROF_D_POSE,     //   the skeleton: limb matrices and display lists (includes the callbacks)
    FISHMOD_PROF_D_LIMB_CB,  //     the limb callbacks (part scales, tints, head position)
    FISHMOD_PROF_D_WHISKERS, //   catfish whiskers
    FISHMOD_PROF_D_BILLBOARD,//   far fish drawn flat
    FISHMOD_PROF_U_STEER,    //   (in AI) steering toward a target, staying in the water
    FISHMOD_PROF_U_SPOOK,    //   (in AI) checking for splashes and Link
    FISHMOD_PROF_U_SEE,      //   (in AI) open water between it and a point (noticing the bobber, escapes)
    FISHMOD_PROF_U_UNSTICK,  //   (in AI) the stuck watchdog
    FISHMOD_PROF_D_SCATTER,  // the habitat props (drawn by the fish manager)
    FISHMOD_PROF_D_PARTICLES, // underwater bubbles and specks
    FISHMOD_PROF_MAX
} FishmodProfSlot;
extern OSTime gFishmodProf[FISHMOD_PROF_MAX];
extern OSTime gFishmodLastProf[FISHMOD_PROF_MAX]; // over the last frame
// the hardware bench (fishmod_debug.c, FISHMOD_HWBENCH) switches these off to time the rest without them
#define FISHMOD_BENCH_NOPROPS (1 << 0)
#define FISHMOD_BENCH_NOFISH (1 << 1)
#define FISHMOD_BENCH_ALTEDGE (1 << 2)    // the props' other edge mode (anti-aliased in the RDP, or by the VI only)
#define FISHMOD_BENCH_NOZUPD (1 << 3)     // the props don't write the depth buffer
#define FISHMOD_BENCH_1CYCLE (1 << 4)     // the props in 1-cycle mode (no fog, no tint): pipeline or memory bound?
#define FISHMOD_BENCH_NONEAR (1 << 5)     // props within FISHMOD_BENCH_NEAR not drawn
#define FISHMOD_BENCH_NOFAR (1 << 6)      // props past it not drawn
#define FISHMOD_BENCH_NOSKY (1 << 7)      // no skybox
#define FISHMOD_BENCH_NOROOMXLU (1 << 8)  // no room translucent geometry (the water surface)
#define FISHMOD_BENCH_NEAR 500.0f
#define FISHMOD_BENCH_SKIP(flag) ((gFishmodBench & (flag)) != 0)
extern s32 gFishmodBench;
extern s32 gFishmodScatterDrawn; // props drawn in the last frame
extern u32 gFishmodProfCount[FISHMOD_PROF_MAX];
#define FISHMOD_PROF(slot, stmt)                        \
    {                                                   \
        OSTime profStart_ = osGetTime();                \
        stmt;                                           \
        gFishmodProf[slot] += osGetTime() - profStart_; \
        gFishmodProfCount[slot]++;                      \
    }
extern Vec3f gFishmodDebugFocus; // the fish nearest to Link, for the follow camera
extern s32 gFishmodDebugFocusSchool; // the follow camera follows school leaders only

// The LOD calibration (FISHMOD_LOD_CALIBRATE, fishmod_debug.c): props drawn out to 3000 units, each kind no further
// than dist[kind] (0: 3000), no fish or particles, while frames are compared.
#define FISHMOD_LOD_KINDS 48
typedef struct FishmodLodTest {
    /* 0x00 */ u8 active;
    /* 0x01 */ u8 under;     // the camera is under water (set by the scatter)
    /* 0x02 */ u8 kindCount; // FISHMOD_PROP_KIND_MAX (set by the scatter)
    /* 0x04 */ s16 dist[FISHMOD_LOD_KINDS];
    /* 0x64 */ u16 drawn[FISHMOD_LOD_KINDS]; // per kind, props drawn in the last frame (set by the scatter)
    /* 0xC4 */ u8 asGame; // the rotation check: the props as the game picks them (its distances, cone, caps)
    /* 0xC5 */ u8 wide;   // the rotation check's reference: a much wider cone, nothing cut by distance
} FishmodLodTest;
extern FishmodLodTest gFishmodLodTest;
extern s16 gFishmodDebugFocusYaw;
#else
#define FISHMOD_PROF(slot, stmt) \
    {                            \
        stmt;                    \
    }
#define FISHMOD_BENCH_SKIP(flag) 0
#endif

/* Debug harness (fishmod_debug.c), only built with FISHMOD_DEBUG */

#define FISHMOD_DEBUG_LINES 12
#define FISHMOD_DEBUG_LINE_LEN 40

typedef struct FishmodAutoplayStep {
    /* 0x00 */ u16 frames;
    /* 0x02 */ u16 button;
    /* 0x04 */ s8 stickX;
    /* 0x05 */ s8 stickY;
    /* 0x06 */ u8 inPlay; // count only frames of gameplay (not menus or loading), e.g. to wait for Link to be in
    // If set: instead of holding the button, press it every `frames` frames until the pause menu's state is
    // untilPause - 1 (e.g. PAUSE_STATE_MAIN + 1: until the pause menu is open and ready)
    /* 0x07 */ u8 untilPause;
} FishmodAutoplayStep; // size = 0x8

#include "fishmod_watermap.h"
// The scene's baked water floor map, loaded into the zelda arena (free it with ZeldaArena_Free), or NULL if it has
// none
FishmodWaterMapHeader* Fishmod_LoadWaterMap(s16 sceneId);
#include "fishmod_scatter.h"
// The scene's scattered habitat props (fishmod/tools/gen_scatter.py), or NULL
FishmodScatterHeader* Fishmod_LoadScatter(s16 sceneId);
// Adds the scene's habitat-prop collision (rocks) to its collision header, before BgCheck reads it
struct CollisionHeader;
void Fishmod_AddPropCollision(struct PlayState* play, struct CollisionHeader* colHeader);

// Time: the 20 minute day and the hidden clock for places where time stands still (fishmod.c)
void Fishmod_AdvanceTime(struct PlayState* play);
void Fishmod_TimeUpdate(struct PlayState* play);
void Fishmod_TimeOnSceneExit(void);

// Weather and light (fishmod.c)
typedef enum FishmodWeather {
    /* 0 */ FISHMOD_WEATHER_CLEAR,
    /* 1 */ FISHMOD_WEATHER_OVERCAST,
    /* 2 */ FISHMOD_WEATHER_RAIN
} FishmodWeather;
s32 Fishmod_WeatherRoll(s16 sceneId, s32 day);
void Fishmod_OnEnvironmentInit(struct PlayState* play);
s32 Fishmod_Weather(struct PlayState* play);
f32 Fishmod_Light(struct PlayState* play);
// Underwater haze, 0 (camera above water) to 1 (under) (FISHMOD_OPT 5)
extern f32 gFishmodHaze;
// the props' textures in 4 bits instead of 8 (half the memory read per texture load): a switch for measuring
extern s32 gFishmodTexCompress;
// the RDP's hardware counters over the last frame (Fishmod_SampleRdp; RDP clock cycles, 24 bits)
typedef struct FishmodRdpCounters {
    u32 clock;
    u32 buf;
    u32 pipe;
    u32 tmem;
} FishmodRdpCounters;
extern FishmodRdpCounters gFishmodRdp;
void Fishmod_SampleRdp(void);
// where the underwater fog is complete (of 1000; vanilla: 1000, the far plane)
#define FISHMOD_HAZE_FOG_FAR 990

#if FISHMOD_DEBUG
void Fishmod_DebugBoot(void);
// True if the autoplay script wants a normal boot (logo, title, file select) instead of going straight to Kokiri
s32 Fishmod_DebugNormalBoot(void);
s32 Fishmod_DebugWantsFish(void);
// Fish per scene roster wanted by the autoplay script (benchmarks), 0 = the game's own count
s32 Fishmod_DebugRosterCount(void);
void Fishmod_AutoplayInput(struct GameState* gameState, Input* inputs);
void Fishmod_DebugPlayUpdate(struct PlayState* play);
void Fishmod_DebugDrawText(struct GameState* gameState, Gfx** gfxP);
void Fishmod_DebugPrintf(s32 line, const char* fmt, ...);
#endif

#endif
