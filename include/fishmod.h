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
    /* 2 */ FISHMOD_BASE_JABU
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
    /* 6 */ FISHMOD_BAIT_MAX
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
    /* 0x14 */ u16 weightCoef; // weight (lbs) = weightCoef * length^3 / 100000
    /* 0x16 */ FishmodBehavior behavior;
    /* 0x1F */ u8 bait[FISHMOD_BAIT_MAX]; // interest multiplier percent per bait
    /* 0x25 */ u8 partTint[FISHMOD_PART_MAX][3];  // RGB per part
    /* 0x3A */ u8 partScale[FISHMOD_PART_MAX][3]; // percent per part: length, height, width
    /* 0x4F */ u8 lookFlags;                       // FISHMOD_LOOK_*
} FishmodSpecies;

typedef struct FishmodSpawnEntry {
    /* 0x00 */ u8 species;
    /* 0x01 */ u8 weight;
} FishmodSpawnEntry; // size = 0x02

typedef struct FishmodSceneSpawns {
    /* 0x00 */ s16 sceneId;
    /* 0x02 */ s16 count;
    /* 0x04 */ const FishmodSpawnEntry* entries;
} FishmodSceneSpawns; // size = 0x08

extern const FishmodSpecies gFishmodSpecies[FISHMOD_SPECIES_MAX];
extern const FishmodSceneSpawns gFishmodSceneSpawns[];
extern const s32 gFishmodSceneSpawnsCount;

/* Core hooks (fishmod.c) */

typedef enum FishmodActorType {
    /* 0 */ FISHMOD_TYPE_ROD, // one per scene: rod, line, bobber, camera, fish spawning
    /* 1 */ FISHMOD_TYPE_FISH
} FishmodActorType;

#define FISHMOD_PARAMS(type, species) (((type) << 8) | (species))
#define FISHMOD_GET_TYPE(params) (((params) >> 8) & 0xFF)
#define FISHMOD_GET_SPECIES(params) ((params) & 0xFF)

const FishmodSceneSpawns* Fishmod_GetSceneSpawns(s16 sceneId);
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
} FishmodPhoto; // size = 0x18

extern FishmodPhoto gFishmodPhoto;

/* Debug harness (fishmod_debug.c), only built with FISHMOD_DEBUG */

#define FISHMOD_DEBUG_LINES 8
#define FISHMOD_DEBUG_LINE_LEN 40

typedef struct FishmodAutoplayStep {
    /* 0x00 */ u16 frames;
    /* 0x02 */ u16 button;
    /* 0x04 */ s8 stickX;
    /* 0x05 */ s8 stickY;
} FishmodAutoplayStep; // size = 0x6

#if FISHMOD_DEBUG
void Fishmod_DebugBoot(void);
void Fishmod_AutoplayInput(struct GameState* gameState, Input* inputs);
void Fishmod_DebugPlayUpdate(struct PlayState* play);
void Fishmod_DebugDrawText(struct GameState* gameState, Gfx** gfxP);
void Fishmod_DebugPrintf(s32 line, const char* fmt, ...);
#endif

#endif
