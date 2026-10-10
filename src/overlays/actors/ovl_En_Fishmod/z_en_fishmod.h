#ifndef Z_EN_FISHMOD_H
#define Z_EN_FISHMOD_H

#include "ultra64.h"
#include "actor.h"
#include "fishmod.h"

#define EN_FISHMOD_MAX_LIMBS 16
#define EN_FISHMOD_POSE_GFX_MAX 72

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
#if FISHMOD_OPT >= 2
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
} FishmodRod;

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
void EnFishmod_WaterGridDestroy(struct PlayState* play);
s32 EnFishmod_WaterFloor(struct PlayState* play, f32 x, f32 z, f32* surface, f32* floor, s32 conservative);

s32 EnFishmod_FindWaterPoint(struct PlayState* play, Vec3f* center, f32 radius, f32 minDepth, Vec3f* out,
                             f32* outSurface, f32* outFloor);

#endif
