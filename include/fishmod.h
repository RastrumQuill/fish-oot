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
#define FISHMOD_LOOK_KOI (1 << 1)    // red and black blotches, placed by each fish's looks (its top two bits)
#define FISHMOD_LOOK_CURLS (1 << 2)  // all body: curls hard into its turns, and lies curled when it rests
// the base model's textures are 32x32 (the loach's), else 16x16
#define FISHMOD_BASE_TEX_SIZE(base) (((base) == FISHMOD_BASE_LOACH) ? 32 : 16)

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
    /* 0 */ FISHMOD_TYPE_ROD, // one per scene: rod, line, bobber, camera (any scene, while the rod is out)
    /* 1 */ FISHMOD_TYPE_FISH,
    /* 2 */ FISHMOD_TYPE_COMB, // a chunk of a broken beehive with larvae in it, scooped up with a bottle
    /* 3 */ FISHMOD_TYPE_MANAGER, // one per scene: the fish roster, drawing all the fish
    /* 4 */ FISHMOD_TYPE_UNUSED   // (not a params value) one that couldn't start: killed in Init, nothing to undo
} FishmodActorType;

#define FISHMOD_PARAMS(type, species) (((type) << 8) | (species))
#define FISHMOD_GET_TYPE(params) (((params) >> 8) & 3)
// a fish from the scene's roster: its record index, in bits 10-14
#define FISHMOD_PARAMS_ROSTER(species, index) (FISHMOD_PARAMS(FISHMOD_TYPE_FISH, species) | (((index) + 1) << 10))
#define FISHMOD_GET_ROSTER(params) ((((params) >> 10) & 0x1F) - 1)
#define FISHMOD_GET_SPECIES(params) ((params) & 0xFF)

const FishmodSceneSpawns* Fishmod_GetSceneSpawns(s16 sceneId);
// Which of a scene's roster fish were kept today: they stay gone until the day changes.
s32 Fishmod_RosterIsCaught(s16 sceneId, s32 day, s32 index);
void Fishmod_RosterSetCaught(s16 sceneId, s32 day, s32 index);
void Fishmod_OnSceneLoad(struct PlayState* play);
void Fishmod_PlayUpdate(struct PlayState* play);
s32 Fishmod_CanUseRod(struct PlayState* play);
// how much faster Link pushes and pulls blocks with his strength upgrade (1 without one)
f32 Fishmod_PushSpeedScale(void);
// how far casts reach in a scene, against Lake Hylia's
f32 Fishmod_CastSceneScale(s16 sceneId);

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

// Whether the photo booth / animation studio is running, for the game's own code: in a release build always off, so
// what's under it is compiled out
#if FISHMOD_DEBUG
#define FISHMOD_PHOTO_ON (gFishmodPhoto.active)
#define FISHMOD_STUDIO_ON (gFishmodStudio.active)
#else
#define FISHMOD_PHOTO_ON 0
#define FISHMOD_STUDIO_ON 0
#endif

// The animation check (debug, fishmod/scripts/animcheck.inc.c; fishmod/docs/anim-verify.md): Link held still and posed
// by the harness, one case a frame, his drawing walked and hashed (fishmod_debug_animcheck.inc.c). Used with
// gFishmodPhoto.active (only Link is drawn).
typedef struct FishmodAnimCheck {
    /* 0x00 */ u8 active;
    /* 0x04 */ void* opaStart; // (Gfx*) where Link's commands start in this frame's lists
    /* 0x08 */ void* xluStart;
    /* 0x0C */ void* opaEnd; // (and where they end: for fishmod/tools/dlstat.py --start/--end on a RAM dump)
    /* 0x10 */ void* xluEnd;
    /* 0x14 */ struct Actor* target; // the actor checked (Link, or the one the animcheck_<object> scripts spawn): only it
                                     // drawn; its update skipped (Link's: Fishmod_AnimCheckPlayerUpdate)
} FishmodAnimCheck;

extern FishmodAnimCheck gFishmodAnimCheck;

#if FISHMOD_DEBUG
#define FISHMOD_ANIMCHECK_ON (gFishmodAnimCheck.active)
#else
#define FISHMOD_ANIMCHECK_ON 0
#endif
struct Player;
struct Actor;
s32 Fishmod_AnimCheckPlayerUpdate(struct Player* player, struct PlayState* play);
void Fishmod_AnimCheckDrawBegin(struct PlayState* play);
void Fishmod_AnimCheckDrawEnd(struct PlayState* play, struct Actor* actor);

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
 * Saved fishing data. The save file has scene flags for 124 scenes; this build has 101 scenes (the debug ROM's test
 * maps make 110), so the entries from 110 on are never used by anything: fishmod keeps its data at scene 110
 * (fixed, whatever the build has). Builds before that kept it at scene 101, where it's moved from once on load
 * (Fishmod_InitSaveExtras). A new file reads as all zeros, which means "nothing caught yet".
 */
#define FISHMOD_SAVE_SCENE 110
#define FISHMOD_SAVE_OLD_SCENE 101 // where builds before the move kept it
#define FISHMOD_SAVE_MAGIC 0x464D  // 'FM': the block has been set up (moved, or begun) by this build or a later one
#define FISHMOD_SAVE_VERSION 2 // 2: the D-pad's buttons
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
    /* 0x00 */ u16 magic;   // FISHMOD_SAVE_MAGIC (zero in saves from before the move)
    /* 0x02 */ u8 version;  // FISHMOD_SAVE_VERSION it was last brought up to
    /* 0x03 */ u8 flags;    // (a u32 before: its low byte, so old flags read the same)
    /* 0x04 */ FishmodSpeciesRecord records[FISHMOD_SAVE_SPECIES_MAX];
    /* 0x84 */ u8 baitCount[FISHMOD_BAIT_SLOTS];  // the bait pouch
    /* 0x8C */ FishmodBottleFish bottleFish[6];    // per bottle (FISHMOD_BOTTLE_INDEX)
    /* 0xA4 */ u8 items[6];                         // the item page's slots past the vanilla 24 (INV_SLOT)
    /* 0xAA */ u8 dpad[2][4]; // the D-pad's buttons (up, down, left, right) for each age (LINK_AGE_ADULT, _CHILD, as
                              // the C buttons have one set each): a slot as the C buttons keep one, an equipment
                              // mark (FISHMOD_SLOT_EQUIP), or SLOT_NONE
} FishmodSaveData; // size = 0xB2

// FishmodSaveData.flags
#define FISHMOD_SAVE_FLAG_ITEMS (1 << 0) // items[] set up (a save from before has zeros there, the Deku Stick's id)
#define FISHMOD_SAVE_FLAG_DPAD (1 << 1)  // dpad[] set up (version 2)

#define FISHMOD_SAVE ((FishmodSaveData*)&gSaveContext.save.info.sceneFlags[FISHMOD_SAVE_SCENE])

/*
 * The item page (fishmod/docs/roadmap.md, mockup C): six columns and five rows. The vanilla 24 slots keep their
 * numbers and their place in the save; the six new ones live in FishmodSaveData.items. Every slot is read and
 * written through INV_SLOT(slot). The grid places slots by FISHMOD_ITEM_GRID (z_kaleido_item.c):
 *     stick     nut       bomb      bow       fire arrow  Din's
 *     slingshot ocarina   bombchu   hookshot  ice arrow   Farore's
 *     boomerang lens      beans     hammer    light arrow Nayru's
 *     ROD       LURE 1    LURE 2    LURE 3    adult trade child trade
 *     bottle 1  bottle 2  bottle 3  bottle 4  BOTTLE 5    BOTTLE 6
 */
// The debug timers' clock: the CPU's count register, read directly (osGetTime turns interrupts off and on and adds the
// 64-bit time to it every call: hundreds of calls a frame cost a few percent of the CPU in the bench builds). Only
// differences are taken, so its wrapping every 91 seconds doesn't matter: the same differences as osGetTime's.
#define FISHMOD_CLOCK() ((OSTime)osGetCount())
#define FISHMOD_SINCE(start) ((u32)(osGetCount() - (u32)(start)))

// A seed from the clock, except in scripted debug runs, which must replay exactly (fishmod/tools/state_hash.py)
#ifdef FISHMOD_SCRIPT
#define FISHMOD_TIME_SEED() ((u32)0x46495348)
#else
#define FISHMOD_TIME_SEED() ((u32)osGetTime())
#endif

/*
 * The mod's own random numbers (fishmod.c), so it never draws from or reseeds the game's: vanilla's randomness
 * doesn't depend on how many fish there are, and each fish of the roster is made from a stream of its own (its scene,
 * the day and its slot), so a change to one fish never reshuffles the others.
 */
typedef struct FishmodRng {
    u32 state;
} FishmodRng;

extern FishmodRng gFishmodRng;     // the fish's play: seeded as each scene loads (fixed in scripted debug runs)
extern FishmodRng* gFishmodRngNow; // the stream the mod draws from: gFishmodRng, or a roster fish's while it's made
void Fishmod_RandSeed(FishmodRng* rng, u32 seed);
u32 Fishmod_RandNext(FishmodRng* rng);
f32 Fishmod_RandZeroOne(FishmodRng* rng);
f32 Fishmod_RandZeroFloat(FishmodRng* rng, f32 f);
f32 Fishmod_RandCenteredFloat(FishmodRng* rng, f32 f);

#define SLOT_FISHING_ROD 24
#define SLOT_LURE_1 25
#define SLOT_LURE_2 26
#define SLOT_LURE_3 27
#define SLOT_BOTTLE_5 28
#define SLOT_BOTTLE_6 29
#define INVENTORY_SLOTS 30
#define INVENTORY_SLOTS_VANILLA 24

// Every item slot's byte, through Fishmod_InvSlot: a slot past the inventory (SLOT_NONE, or an equipment button's
// mark, FISHMOD_SLOT_EQUIP) gets a spare byte that reads as no item, so code that looks a button's slot up is safe
#define INV_SLOT(slot) (*Fishmod_InvSlot(slot))
u8* Fishmod_InvSlot(s32 slot);

// A C or D-pad button can hold a piece of equipment (a sword, shield, tunic or boots): its slot is this mark plus the
// equipment item's place after ITEM_SWORD_KOKIRI (fishmod_items.c: pressing it equips it)
// An equipment button (C or D-pad): FISHMOD_SLOT_EQUIP + type * 4 + value (EQUIP_TYPE_*, EQUIP_VALUE_* 1-3); its item
// is ITEM_SWORD_KOKIRI + type * 3 + value - 1
#define FISHMOD_SLOT_EQUIP 0x80
#define FISHMOD_IS_EQUIP_SLOT(slot) (((slot) >= FISHMOD_SLOT_EQUIP) && ((slot) < FISHMOD_SLOT_EQUIP + 16))
#define FISHMOD_EQUIP_SLOT(type, value) (FISHMOD_SLOT_EQUIP + (type) * 4 + (value))

// Item buttons: 0 B, 1-3 the C buttons, 4-7 the D-pad (up, down, left, right)
#define FISHMOD_BUTTON_DPAD 4
#define FISHMOD_BUTTON_MAX 8
u8 Fishmod_ButtonSlot(s32 button);
u8 Fishmod_SlotItem(s32 slot);
s32 Fishmod_DpadItem(struct PlayState* play, s32 button);
s32 Fishmod_IsEquipItem(s32 item);
s32 Fishmod_EquipItem(struct PlayState* play, s32 item);
void Fishmod_EquipButtons(struct PlayState* play, struct Input* input); // every frame (z_player.c)
void Fishmod_DpadUpdateBottle(struct PlayState* play, u8 item, s32 button);
s32 Fishmod_DpadAssign(u16 press, s32 slot);
void Fishmod_DpadDraw(struct PlayState* play);
// The HUD clock under the overworld minimap (fishmod_items.c)
extern s16 gFishmodMinimapLift;
s32 Fishmod_ClockLift(s32 mapBottom);
void Fishmod_ClockDraw(struct PlayState* play, s32 centerX, s32 top, s16 alpha);

// Bottles: six, numbered 0-5 (bottles 1-4 are the vanilla slots, 5 and 6 the new ones)
#define FISHMOD_BOTTLES 6
#define FISHMOD_BOTTLE_SLOT(i) (((i) < 4) ? (SLOT_BOTTLE_1 + (i)) : (SLOT_BOTTLE_5 + (i) - 4))
#define FISHMOD_IS_BOTTLE_SLOT(slot) \
    ((((slot) >= SLOT_BOTTLE_1) && ((slot) <= SLOT_BOTTLE_4)) || (((slot) >= SLOT_BOTTLE_5) && ((slot) <= SLOT_BOTTLE_6)))
#define FISHMOD_BOTTLE_INDEX(slot) (((slot) >= SLOT_BOTTLE_5) ? ((slot) - SLOT_BOTTLE_5 + 4) : ((slot) - SLOT_BOTTLE_1))

// Lures: items of their own (placeholders for now: they go on a C button, but using one does nothing yet)
#define ITEM_LURE_CRANKBAIT ITEM_INVALID_4
#define ITEM_LURE_FROG ITEM_INVALID_5
#define ITEM_LURE_SINKING ITEM_INVALID_6
#define FISHMOD_IS_LURE_ITEM(item) (((item) >= ITEM_LURE_CRANKBAIT) && ((item) <= ITEM_LURE_SINKING))

// The mod's items (fishmod_items.c): what Link does with one from a button; whether an item is a bottle
s32 Fishmod_ItemAction(s32 item);
s32 Fishmod_IsBottleItem(s32 item);
// The item page's grid: the slot at each place, and each slot's place
extern u8 gFishmodItemGridSlot[];
extern u8 gFishmodItemSlotGrid[];

// Sets up the save block's new parts (a new file, or one saved before they existed)
void Fishmod_InitSaveExtras(void);
// The item page's own graphics (fishmod_item_page.h), loaded with the pause menu's: where to load them, and once
// loaded, the page's tiles in place of vanilla ones
struct FishmodItemPage;
extern struct FishmodItemPage* gFishmodItemPageData;
u32 Fishmod_ItemPageSize(void);
void Fishmod_LoadItemPage(struct PlayState* play, void* dst);
// The item's icon in ROM (fishmod's items whose icons aren't in RAM), or 0
uintptr_t Fishmod_ItemIconVrom(s32 item);
// The item's icon texture while paused (the lures' are in the item page's graphics)
void* Fishmod_PauseItemIcon(s32 item);
// The item's name graphic in ROM if fishmod has one of its own, or 0
uintptr_t Fishmod_ItemNameVrom(s32 item);

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
s32 Fishmod_UseItem(struct PlayState* play, s32 item, s32 button);
void Fishmod_ResetRodInfo(void);
s32 Fishmod_SceneHasFish(struct PlayState* play);
void Fishmod_SizeCollision(s16 sceneId, u32* memSize, Vec3i* subdivAmount);
struct GraphicsContext;
void Fishmod_ClaimDisplayLists(struct GraphicsContext* gfxCtx);
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
// The update's steps, timed one by one (Play_Update, Actor_UpdateAll) for the area bench (FISHMOD_BENCH_AREA): summed
// over a frame, cleared by whoever reads them
typedef enum FishmodSect {
    FISHMOD_SECT_FM_PLAY,    // Fishmod_PlayUpdate (the mod's per-frame work outside its actors)
    FISHMOD_SECT_FM_DEBUG,   // Fishmod_DebugPlayUpdate (the debug harness and the benches themselves)
    FISHMOD_SECT_OBJECTS,    // Object_UpdateEntries
    FISHMOD_SECT_ROOMREQ,    // Room_ProcessRoomRequest
    FISHMOD_SECT_COL_AT,     // CollisionCheck_AT
    FISHMOD_SECT_COL_OC,     // CollisionCheck_OC
    FISHMOD_SECT_COL_DAMAGE, // CollisionCheck_Damage
    FISHMOD_SECT_COL_CLEAR,  // CollisionCheck_ClearContext
    FISHMOD_SECT_ACTORS,     // Actor_UpdateAll (all of it)
    FISHMOD_SECT_DYNAPOLY,   //   DynaPoly_UpdateContext (inside it)
    FISHMOD_SECT_CUTSCENE,   // Cutscene_UpdateManual + Scripted
    FISHMOD_SECT_EFFECTS,    // Effect_UpdateAll + EffectSs_UpdateAll
    FISHMOD_SECT_ROOMFX,     // the rooms' per-frame updates (func_80095AA0)
    FISHMOD_SECT_SKYBOX,     // Skybox_Update
    FISHMOD_SECT_MESSAGE,    // Message_Update (or the pause menu, or game over)
    FISHMOD_SECT_INTERFACE,  // Interface_Update
    FISHMOD_SECT_ANIMTASK,   // AnimTaskQueue_Update
    FISHMOD_SECT_SFXSRC,     // SfxSource_UpdateAll
    FISHMOD_SECT_CAMERA,     // Camera_Update (every camera)
    FISHMOD_SECT_ENV,        // Environment_Update
    FISHMOD_SECT_MAX
} FishmodSect;
#define FISHMOD_ACTOR_IDS 480 // (ACTOR_ID_MAX is 471)
#define FISHMOD_ACTOR_CATS 12
extern OSTime gFishmodSect[FISHMOD_SECT_MAX];
extern u32 gFishmodActorCycles[FISHMOD_ACTOR_IDS]; // each actor kind's updates this frame (CPU count cycles)
extern u8 gFishmodActorCount[FISHMOD_ACTOR_IDS];
extern u64 gFishmodBgCycles[FISHMOD_ACTOR_IDS]; // each actor kind's Actor_UpdateBgCheckInfo, the whole run (cycles)
extern u32 gFishmodBgCalls[FISHMOD_ACTOR_IDS];   // and how many of it updated
extern u32 gFishmodCatCycles[FISHMOD_ACTOR_CATS];
#define FISHMOD_SECT(slot, stmt)                          \
    {                                                     \
        OSTime sectStart_ = FISHMOD_CLOCK();                  \
        stmt;                                             \
        gFishmodSect[slot] += FISHMOD_SINCE(sectStart_);   \
    }

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
    FISHMOD_PROF_D_RIG_BONES, //   (in pose) the rig's bone frames: the swim wave's angles, the body and fin bones
    FISHMOD_PROF_D_RIG_MTX,   //   (in pose) the bones' matrices converted for the RSP
    FISHMOD_PROF_D_RIG_LOOK,  //   (in pose) the fish's own colours (its tint table)
    FISHMOD_PROF_U_ATTN,      // (in update) how far it is from attention (the camera, the bobber)
    FISHMOD_PROF_U_FOLLOW,    // (in update) a far school member taking its place behind its leader
    FISHMOD_PROF_MAX
} FishmodProfSlot;
extern OSTime gFishmodProf[FISHMOD_PROF_MAX];
// The PC profile's samples tagged by what the game was doing when they were taken (fishmod/tools/pc_ram.py --tag):
// whatever ran then counts, a thread that took over included (the audio thread preempting a fish's update shows as
// audio under the fish's tag)
extern u32 gFishmodPcTag;
extern u32 gFishmodScatterDropped[4];
// The flat fish compared (fishmod_manager.c, LIT_DIST): set by a script (FISHMOD_FLAT_TEST_INIT), else normal
extern s32 gFishmodFlatTest;
#define FISHMOD_FLAT_TEST gFishmodFlatTest
#define FISHMOD_PCTAG_FISH_UPDATE 1 // a fish's update (Actor_UpdateAll)
extern OSTime gFishmodLastProf[FISHMOD_PROF_MAX]; // over the last frame
// the hardware bench (fishmod_debug.c, FISHMOD_HWBENCH) switches these off to time the rest without them
#define FISHMOD_BENCH_NOPROPS (1 << 0)
#define FISHMOD_BENCH_NOOCC (1 << 3)     // no occlusion plane (fishmod_occlusion.c)
#define FISHMOD_BENCH_NOBEDCARD (1 << 4) // no bed cards: distant clumps drawn plant by plant
#define FISHMOD_BENCH_NOFISH (1 << 1)
#define FISHMOD_BENCH_ALTEDGE (1 << 2)    // the props' old edge mode (anti-aliased in the RDP, reading the colour buffer)
#define FISHMOD_BENCH_TWOCYCLE (1 << 5)   // the props' old 2-cycle drawing (the light in the combiner's second cycle, fog in the blender's first)
#define FISHMOD_BENCH_NOIMP (1 << 12)      // no far single cards: the whole cards out to the cutoff
#define FISHMOD_BENCH_NOFARTEX (1 << 13)   // far props on their full-size textures
#define FISHMOD_BENCH_NODISSOLVE (1 << 14) // the single cards pop at the cutoff instead of dissolving
#define FISHMOD_BENCH_SKYUNDER (1 << 16)   // the skybox drawn with the camera under water too, as vanilla
#define FISHMOD_BENCH_NOFISHAI (1 << 20)   // the fish frozen: no thinking or swimming (still drawn)
#define FISHMOD_BENCH_OLDSKY_FILL (1 << 21) // frames11's sky A/B (sky_ab): the colour fill kept under the sky,
#define FISHMOD_BENCH_OLDSKY_Z (1 << 22)    //   the Z clear by the RDP at the frame's start,
#define FISHMOD_BENCH_OLDSKY_1CYC (1 << 23) //   the sky in 2-cycle always
#define FISHMOD_BENCH_OLDSKY (FISHMOD_BENCH_OLDSKY_FILL | FISHMOD_BENCH_OLDSKY_Z | FISHMOD_BENCH_OLDSKY_1CYC)
#define FISHMOD_BENCH_NOSKY (1 << 7)      // no skybox
#define FISHMOD_BENCH_NOROOMXLU (1 << 8)  // no room translucent geometry (the water surface)
#define FISHMOD_BENCH_NOHUD (1 << 25)     // no HUD or minimap (the RDP sweep: fishmod/scripts/rdp_sweep.inc.c)
#define FISHMOD_BENCH_SKIP(flag) ((gFishmodBench & (flag)) != 0)
extern s32 gFishmodBench;
extern struct Actor* gFishmodDrawingActor; // debug builds: the actor Actor_Draw is drawing (the crash record says which)
// the benches and the profile: leave out debug-only work that the shipped game doesn't do
extern s32 gFishmodTimingAsShipped;
extern s32 gFishmodBenchDone;
s32 Fishmod_RdpBenchDraw(struct PlayState* play); // the RDP calibration bench's frame (debug builds) // the bench route's last lap is over (the profiler stops)
// the stress benches (fishmod_debug_stress.inc.c): 1 fish, 2 plants, 0 none; and the step's count of them
extern s32 gFishmodStress;
extern s32 gFishmodStressCount;
extern s32 gFishmodScatterDrawn; // props drawn in the last frame
extern u32 gFishmodScatterCacheUsed;
extern u32 gFishmodScatterCacheMismatch;
extern u32 gFishmodScatterCacheDiff[8];
extern u32 gFishmodScatterCmdTime[2];
extern u32 gFishmodProfCount[FISHMOD_PROF_MAX];
#define FISHMOD_PROF(slot, stmt)                        \
    {                                                   \
        OSTime profStart_ = FISHMOD_CLOCK();                \
        stmt;                                           \
        gFishmodProf[slot] += FISHMOD_SINCE(profStart_); \
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
    // the LOD tuning (FISHMOD_LOD_TUNE): from how far (at scale 1) each kind's far single card and half-size texture
    // are used (0: not at all; the reference uses neither, and nothing dissolves past the cutoff while testing)
    /* 0xC6 */ s16 imp[FISHMOD_LOD_KINDS];
    /* 0x126 */ s16 half[FISHMOD_LOD_KINDS];
    /* 0x188 */ u64 hasImp;  // the kinds that have a single card (set by the scatter)
    /* 0x190 */ u64 hasHalf; // and a half-size texture
} FishmodLodTest;
extern FishmodLodTest gFishmodLodTest;
extern s16 gFishmodDebugFocusYaw;
#else
#define FISHMOD_PROF(slot, stmt) \
    {                            \
        stmt;                    \
    }
#define FISHMOD_FLAT_TEST 0
#define FISHMOD_SECT(slot, stmt) \
    {                            \
        stmt;                    \
    }
#define FISHMOD_BENCH_SKIP(flag) 0
#define FISHMOD_ZBUFFER ((u16*)gZBuffer)
#endif
// With the camera under water (the haze full) the skybox can't be seen: not drawn, and the frame cleared to the
// fog's colour instead of black
#define FISHMOD_NO_SKY() ((gFishmodHaze >= 1.0f) && !FISHMOD_BENCH_SKIP(FISHMOD_BENCH_SKYUNDER))

/* Debug harness (fishmod_debug.c), only built with FISHMOD_DEBUG */

#define FISHMOD_DEBUG_LINES 12
#define FISHMOD_DEBUG_LINE_LEN 40

#define FISHMOD_FLAT_TEST_ALL_LIT 1  // every fish lit (the model's own material), as before the flat fish
#define FISHMOD_FLAT_TEST_ALL_FLAT 2 // every fish flat, even up close

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
#if FISHMOD_DEBUG
extern s32 gFishmodNoPropCol; // the debug harness can leave the props' collision out (FISHMOD_SCRIPT_NO_PROPCOL)
#endif

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
// Underwater haze, 0 (camera above water) to 1 (under)
extern f32 gFishmodHaze;
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
void Fishmod_DebugColNodes(s32 sceneId, s32 used, s32 max);
// A crash or a hang, written to SRAM 0x7C00 ('FFLT') so the save says where it happened (fault_n64.c calls it)
void Fishmod_DebugFault(struct OSThread* thread, const char* exp1, const char* exp2);
#endif

struct Lights;
// the lake's terrain lit by the CPU (fishmod.c, meshpack builds): sets its vertex segment for this frame
Gfx* Fishmod_Relight(struct PlayState* play, struct Lights* lights, Gfx* gfx);

#endif
