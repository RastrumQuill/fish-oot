#ifndef Z_EN_FISHMOD_H
#define Z_EN_FISHMOD_H

#include "ultra64.h"
#include "actor.h"
#include "fishmod.h"

#define EN_FISHMOD_MAX_LIMBS 16
#define EN_FISHMOD_POSE_GFX_MAX 72

#define FISHMOD_FISH_COUNT 10       // fish per fishing scene
#if FISHMOD_OPT >= 5
#define FISHMOD_ROSTER_MAX 600      // the most a scene's roster can hold
#else
#define FISHMOD_ROSTER_MAX 30       // (FISHMOD_OPT 4) the most a scene's roster can hold
#endif
#define FISHMOD_NO_ROSTER 0xFFFF
#if FISHMOD_OPT >= 5
#define FISHMOD_LOD_DIST 750.0f     // farther from the camera than this: the flat far shape (textured)
#define FISHMOD_CULL_DIST 1300.0f   // and past this not drawn at all
#else
#define FISHMOD_LOD_DIST 380.0f     // (FISHMOD_OPT 3) farther from the camera than this: the flat far shape
#define FISHMOD_CULL_DIST 900.0f
#endif
#define FISHMOD_SMALL_RIG_LENGTH 9.0f // (FISHMOD_OPT 4) fish shorter than this (inches) always use the reduced rig

// Fish rigs (fishmod_rig.c, FISHMOD_OPT 4)
#define FISH_RIG_MAX_SLOTS 12
typedef enum FishRigVariant {
    /* 0 */ FISH_RIG_FULL,    // every moving limb: head, body joints, jaw, pectoral fins
    /* 1 */ FISH_RIG_REDUCED, // head, middle and tail
    /* 2 */ FISH_RIG_VARIANTS
} FishRigVariant;
typedef enum FishRigModel {
    /* 0 */ FISH_RIG_BASS,
    /* 1 */ FISH_RIG_LOACH,
    /* 2 */ FISH_RIG_SMALL,
    /* 3 */ FISH_RIG_MODELS
} FishRigModel;

/**
 * A fish's mood (FISHMOD_OPT 5), 0-255 each. Hunger rises with time (faster in its active hours) and falls when it
 * feeds; fear rises when it's spooked or something disturbs the water near it, and fades. They scale what the AI
 * already does: how far it notices a bait and how keen it is, how readily it bites, how long it rests, how far it
 * strays from home cover and how easily it spooks.
 */
typedef struct FishmodMood {
    u8 hunger;
    u8 fear;
} FishmodMood;

// A fish's own looks table (EnFishmod_LookTable): part colours, then each texture role's render tile. The same
// segment as the rig's species tints (fishmod_rig.c RIG_SEG_TINT), so the rig's colour branches find the fish's own.
#define FISHMOD_SEG_LOOK 0x0F
#define FISHMOD_LOOK_NEUTRAL 0x1B // the species' own colours, pattern as the model has it (the wiki photos)

// The bass's jaw (limb index, 0-based) and how far it turns to shut: the model's rest pose gapes
#define FISHMOD_BASS_JAW_LIMB 12
#if FISHMOD_OPT >= 5
#define FISHMOD_BASS_JAW_SHUT 3800
#else
#define FISHMOD_BASS_JAW_SHUT 0
#endif

// Texture roles: each is drawn from its own segment (0x08 + role) so species can swap in patterned textures.
typedef enum FishmodTexRole {
    /* 0 */ FISHMOD_TEX_ROLE_BACK,
    /* 1 */ FISHMOD_TEX_ROLE_BELLY,
    /* 2 */ FISHMOD_TEX_ROLE_HEAD,
    /* 3 */ FISHMOD_TEX_ROLE_SIDE,
    /* 4 */ FISHMOD_TEX_ROLE_FIN,
    /* 5 */ FISHMOD_TEX_ROLE_MAX
} FishmodTexRole;

typedef enum FishmodFishState {
    /* 0 */ FISH_STATE_WANDER,   // cruising around its home water
    /* 1 */ FISH_STATE_APPROACH, // noticed the bobber, swimming over
    /* 2 */ FISH_STATE_INSPECT,  // hovering under the bobber, deciding
    /* 3 */ FISH_STATE_NIBBLE,   // a quick peck at the bait
    /* 4 */ FISH_STATE_BITE,     // took the bait: the bobber is pulled under
    /* 5 */ FISH_STATE_HOOKED,   // on the line, fighting
    /* 6 */ FISH_STATE_CAUGHT,   // held up by Link
    /* 7 */ FISH_STATE_FLEE,     // spooked
    /* 8 */ FISH_STATE_PHOTO     // posing for the wiki
} FishmodFishState;

struct EnFishmod;

typedef void (*EnFishmodActionFunc)(struct EnFishmod*, struct PlayState*);

#if FISHMOD_OPT >= 3
#define FISHMOD_THINK_MASK(this) ((this)->thinkMask)
#else
#define FISHMOD_THINK_MASK(this) 1
#endif

typedef struct EnFishmod {
    /* 0x000 */ Actor actor;
    /* 0x14C */ SkelAnime skelAnime;
    /* 0x190 */ Vec3s jointTable[EN_FISHMOD_MAX_LIMBS + 1];
    /* 0x1F6 */ Vec3s morphTable[EN_FISHMOD_MAX_LIMBS + 1];
    /* 0x25C */ EnFishmodActionFunc actionFunc;
    /* 0x260 */ const FishmodSpecies* species;
    /* 0x264 */ f32 lengthInches;
    /* 0x268 */ f32 swimPhase;
    /* 0x26C */ f32 swimPhaseStep;
    /* 0x270 */ f32 swimMag;     // tail beat amplitude (binary angle units)
    /* 0x274 */ s16 bodyYaw[4];  // per-segment yaw for the swim wave
    /* 0x27C */ s16 finYaw;      // pectoral fin flutter
    /* 0x27E */ s16 jawRot;      // mouth open
    /* 0x280 */ u8 limbParent[EN_FISHMOD_MAX_LIMBS]; // 0xFF for the root
    /* 0x290 */ Vec3f limbScale[EN_FISHMOD_MAX_LIMBS];
    /* 0x350 */ u8 limbCount;
    /* 0x351 */ u8 type;
    /* 0x352 */ s16 timer;
    /* 0x354 */ Vec3f headPos; // world position of the head, for whiskers and the hook
    /* 0x360 */ Vec3f boundsMin; // world bounds of the limb origins, from the last draw
    /* 0x36C */ Vec3f boundsMax;
    // fish AI
    /* 0x378 */ u8 fishState;   // FishmodFishState
    /* 0x379 */ u8 nibblesLeft;
    /* 0x37A */ s16 stateTimer;
    /* 0x37C */ s16 cooldown;   // frames before it will look at the bobber again
    /* 0x37E */ s16 runYaw;     // direction it runs when hooked
    /* 0x380 */ Vec3f target;
    /* 0x38C */ Vec3f vel;
    /* 0x398 */ f32 speed;
    /* 0x39C */ f32 stamina;    // fight stamina, counts down while it pulls
    /* 0x3A0 */ f32 staminaMax;
    /* 0x3A4 */ f32 waterY;     // water surface above it
    /* 0x3A8 */ f32 floorY;     // floor below it
    /* 0x3AC */ f32 interest;   // builds while it watches the bobber
    /* 0x3B0 */ s16 runTimer;
    /* 0x3B2 */ u8 jumping;
    /* 0x3B3 */ u8 beached;     // reeled up onto the bank: it counts as landed
    /* 0x3B4 */ f32 halfLength; // body size in world units, for keeping it inside the water
    /* 0x3B8 */ f32 halfHeight;
    /* 0x3BC */ u8 limbMode[EN_FISHMOD_MAX_LIMBS]; // EnFishmodLimbMode: the cheapest way to apply the part scales
    /* 0x3CC */ u8 lastTintPart;                   // during a draw: the part whose tint is currently set
    /* 0x3CD */ u8 blocked;                        // the last SwimToward hit the shore (debug display)
    /* 0x3CE */ s16 detourYaw;                     // heading around a bank that was in the way
    /* 0x3D0 */ s16 detourTimer;                   // frames left to follow detourYaw
    /* 0x3D2 */ u8 runKind;                        // EnFishmodRunKind of the current or last run when hooked
    /* 0x3D3 */ u8 stuckLevel;                     // how many checks in a row found it not moving
    /* 0x3D4 */ s16 stuckTimer;                    // frames since the last stuck check
    /* 0x3D6 */ s16 escapeTimer;                   // frames left heading for `escape` to get unstuck
    /* 0x3D8 */ Vec3f stuckPos;                    // where it was at the last stuck check
    /* 0x3E4 */ Vec3f escape;                      // open water to head for when stuck
    /* 0x3F0 */ s16 lastYaw;                       // heading last frame, for bending the body into turns
    /* 0x3F2 */ s16 turnBend;                      // how hard the body is bent into the current turn
    /* 0x3F4 */ s16 restTimer;                     // wandering: frames left resting in place before moving on
    /* 0x3F6 */ s8 startle;                        // frames left of a startled flick (sign: which way)
    /* 0x3F7 */ u8 fleeFromLink;                   // the current flee is away from Link (runYaw already set)
    /* 0x3F8 */ u8 thinkMask;                      // the AI runs on frames where (timer & thinkMask) == 0
    /* 0x3F9 */ u8 depthPref;                      // where in its species' depth band this fish likes to be
    /* 0x3FA */ u16 rosterIndex;                   // its record in the scene's roster, FISHMOD_NO_ROSTER if none
    /* 0x3FC */ s16 lastThink;                     // timer value when it last ran its AI (FISHMOD_OPT 4)
    /* 0x3FE */ s16 startleBend;                   // the C-bend of a startle (sign: which way), 0 otherwise
    /* 0x400 */ u8 rigReady;                       // the manager can draw it (FISHMOD_OPT 4)
    /* 0x404 */ s32 waterCell;                     // the water grid cell waterY/floorY are for (FISHMOD_OPT 5)
    /* 0x408 */ u8 biteStruck;                     // the strike reached the bait (after the wind-up; FISHMOD_OPT 5)
    /* 0x409 */ FishmodMood mood;                  // hunger and fear (kept in its roster record while away; FISHMOD_OPT 5)
    /* 0x40C */ u16 leaderIndex;                   // roster record of its school's leader, FISHMOD_NO_ROSTER if it leads
                                                   // or is on its own (FISHMOD_OPT 5)
    /* 0x40E */ u8 look;                           // its own looks: EnFishmod_LookTable (saved with it in a bottle)
    /* 0x410 */ f32 swimGain;                      // the tail beat's size on top of swimMag (small fish's bursts)
#if (FISHMOD_OPT >= 2) && (FISHMOD_OPT < 4)
    // distant fish redraw their last pose every other frame (see EnFishmod_DrawPose)
    Mtx poseMtx[EN_FISHMOD_MAX_LIMBS + 1];
    Gfx poseGfx[EN_FISHMOD_POSE_GFX_MAX];
    u16 poseGfxCount;
    u16 poseMtxCount;
    u32 poseFrame; // gameplay frame the pose was captured on
#endif
} EnFishmod;

/* Rod controller (fishmod_rod.c) */

typedef enum FishmodRodState {
    /* 0 */ ROD_STATE_NONE,    // Link isn't holding the rod
    /* 1 */ ROD_STATE_HELD,    // rod out, not cast
    /* 2 */ ROD_STATE_WINDUP,  // casting animation, before the bobber leaves
    /* 3 */ ROD_STATE_FLYING,  // bobber in the air
    /* 4 */ ROD_STATE_FLOAT,   // bobber on the water
    /* 5 */ ROD_STATE_GROUND,  // bobber landed on the ground: reel it back
    /* 6 */ ROD_STATE_FIGHT,   // fish on
    /* 7 */ ROD_STATE_LANDING  // fish caught, Link holds it up
} FishmodRodState;

typedef enum EnFishmodRunKind {
    /* 0 */ FISH_RUN_AWAY,   // straight off, away from Link
    /* 1 */ FISH_RUN_ACROSS, // a sweep across the line, bending the rod sideways
    /* 2 */ FISH_RUN_CHARGE  // back toward Link: the line goes slack, reel fast
} EnFishmodRunKind;

typedef struct FishmodRod {
    /* 0x00 */ u8 state;
    /* 0x01 */ u8 biting;     // a fish has taken the bait and the bobber is under
    /* 0x02 */ s16 timer;
    /* 0x04 */ Vec3f bobberPos;
    /* 0x10 */ Vec3f bobberVel;
    /* 0x1C */ Vec3f tipPos;    // rod tip, from the last draw
    /* 0x28 */ f32 waterY;      // surface under the bobber
    /* 0x2C */ f32 dip;         // how far the bobber is pulled under
    /* 0x30 */ f32 dipTarget;
    /* 0x34 */ f32 lineOut;     // line length from the rod tip
    /* 0x38 */ f32 tension;     // 0..100+, the line snaps if it stays over 100
    /* 0x3C */ f32 castPower;
    /* 0x40 */ s16 castYaw;
    /* 0x42 */ s16 biteTimer;   // frames left to set the hook
    /* 0x44 */ s16 overTimer;   // frames the tension has been over the limit
    /* 0x46 */ s16 slackTimer;  // frames the line has been slack during a fight
    /* 0x48 */ s16 twitchTimer; // set when the player twitches the bobber; draws fish in
    /* 0x4A */ u8 bait;         // FishmodBait on the hook
    /* 0x4B */ u8 zoom;         // camera zoom level
    /* 0x4C */ struct EnFishmod* fish; // fish interested in / biting / hooked on the bobber
    /* 0x50 */ s16 camId;
    /* 0x52 */ s16 messageTimer;
    /* 0x54 */ Vec3f camEye;
    /* 0x60 */ Vec3f camAt;
    /* 0x6C */ f32 camBlend;
    /* 0x70 */ char message[2][40]; // banner text
    /* 0xC0 */ u8 messageColor;     // 0 neutral, 1 good, 2 bad
    /* 0xC1 */ u8 splashed;         // set the frame the bobber hits the water
    /* 0xC2 */ u8 castHolding;      // charging a cast
    /* 0xC3 */ u8 choice;           // after a catch: 1 = keep or release, 2 = release only (no empty bottle)
    /* 0xC4 */ f32 lineFloor[17];    // lowest each point along the line can hang: the ground, or the water it rests on
    /* 0x108 */ f32 bendRoll;       // which way the rod bends around its own axis (radians), toward the pull
    /* 0x10C */ f32 bendPull;       // how much of the tension bends the rod: more when the pull is across it
    /* 0x110 */ f32 baitDepth;      // how far under the water the bait hangs (set each cast: see Rod_UpdateSink)
    /* 0x114 */ u8 sinking;         // the bait is still sinking after the cast
    /* 0x115 */ u8 reelLock;        // the A press that stopped the sink: no reeling until A is let go
} FishmodRod;

// The bait's height under the bobber
#define FISHMOD_ROD_BAIT_Y(rod) ((rod)->waterY - (rod)->baitDepth)

extern FishmodRod gFishmodRodState;

void EnFishmod_RodInit(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_RodUpdate(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_RodDraw(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_RodDestroy(struct EnFishmod* this, struct PlayState* play);

// Called by fish
s32 EnFishmod_RodBobberInWater(void);
void EnFishmod_RodNibble(struct EnFishmod* fish);
void EnFishmod_RodBite(struct EnFishmod* fish);
void EnFishmod_RodRelease(struct EnFishmod* fish);
void EnFishmod_RodMessage(s32 color, const char* line1, const char* line2);

// Fish (z_en_fishmod.c), called by the rod
void EnFishmod_FishSetState(struct EnFishmod* this, s32 state);
// Water floor cache (fishmod_water.c)
void EnFishmod_WaterGridInit(struct PlayState* play);
void EnFishmod_WaterGridAcquire(struct PlayState* play);
void EnFishmod_WaterGridRelease(struct PlayState* play);
void EnFishmod_WaterGridDestroy(struct PlayState* play);
void EnFishmod_WaterNewFrame(void);
s32 EnFishmod_WaterFloor(struct PlayState* play, f32 x, f32 z, f32* surface, f32* floor, s32 conservative);
s32 EnFishmod_OpenWaterDir(struct PlayState* play, f32 x, f32 z, s16* yaw);
s32 EnFishmod_WaterCell(struct PlayState* play, f32 x, f32 z);

/* Habitat props (fishmod_scatter.c, FISHMOD_OPT 5) */
void EnFishmod_ScatterLoad(struct PlayState* play);
void EnFishmod_ScatterUnload(void);
s32 EnFishmod_ScatterRandomCover(Vec3f* out);
s32 EnFishmod_ScatterRandomDrawnPlant(Vec3f* eye, f32 range, Vec3f* out);
// Underwater particles: bubbles from the weed, drifting specks (fishmod_particles.c, FISHMOD_OPT 5)
void EnFishmod_ParticlesUpdate(struct PlayState* play);
void EnFishmod_ParticlesDraw(struct PlayState* play);
void EnFishmod_ScatterDraw(struct PlayState* play);

s32 EnFishmod_FindWaterPoint(struct PlayState* play, Vec3f* center, f32 radius, f32 minDepth, Vec3f* out,
                             f32* outSurface, f32* outFloor);

// Fish drawing pieces (z_en_fishmod.c), used by the manager
u8* EnFishmod_PartMap(const FishmodSpecies* species);
Gfx* EnFishmod_SmallDL(Gfx* src);
void EnFishmod_SetSpeciesTextures(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_DrawBillboard(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_DrawPhotoBackdrop(struct PlayState* play);
void EnFishmod_DrawWhiskersRigid(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_PhotoBounds(struct EnFishmod* this);

// Fish rigs (fishmod_rig.c)
s32 EnFishmod_RigModel(const FishmodSpecies* species);
s32 EnFishmod_RigPrepare(struct EnFishmod* this);
void EnFishmod_RigFreeAll(void);
void EnFishmod_RigSetSpecies(struct EnFishmod* this, struct PlayState* play, s32 variant);
void EnFishmod_RigRestoreSegments(struct PlayState* play);
void EnFishmod_RigDraw(struct EnFishmod* this, struct PlayState* play, s32 variant, s32 wantBounds);
void EnFishmod_BillboardMaterial(struct EnFishmod* this, struct PlayState* play);

// Fish manager (fishmod_manager.c)
typedef struct EnFishmodShared {
    u32 frame;
    Vec3f linkPos;
    Vec3f rosterFocus; // where fish are woken around: the camera
    Vec3f camPos;      // the camera (last frame's): fish near it or the bobber get full attention
    Vec3f bobberPos;
    s32 bobberIn;
    s32 thinksLeft;
    f32 light; // Fishmod_Light: 0 dark night - 1 full day (FISHMOD_OPT 5)
    s32 raining;     // (FISHMOD_OPT 5)
    Vec3f splashPos; // the last thing to land in the water (the bobber), and when (FISHMOD_OPT 5)
    u32 splashFrame;
} EnFishmodShared;
EnFishmodShared* EnFishmod_Shared(struct PlayState* play);
#if FISHMOD_OPT >= 5
void EnFishmod_LookTable(struct EnFishmod* this, struct PlayState* play);
struct EnFishmod* EnFishmod_RosterFish(s32 index);
void EnFishmod_Disturb(struct PlayState* play, Vec3f* pos, f32 radius, s32 fear);
#endif
void EnFishmod_ManagerEnsure(struct PlayState* play);
void EnFishmod_ManagerInit(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_ManagerDestroy(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_ManagerUpdate(struct EnFishmod* this, struct PlayState* play);
void EnFishmod_ManagerDraw(struct EnFishmod* this, struct PlayState* play);
s32 EnFishmod_ThinkAllowed(struct EnFishmod* this, struct PlayState* play);
f32 EnFishmod_AttentionDist(struct EnFishmod* this, struct PlayState* play);
f32 EnFishmod_RosterLength(s32 index);
void EnFishmod_RosterCaught(struct PlayState* play, struct EnFishmod* fish);
void EnFishmod_RosterForget(struct EnFishmod* fish);

#endif
