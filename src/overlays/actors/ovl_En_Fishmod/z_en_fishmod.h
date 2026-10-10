#ifndef Z_EN_FISHMOD_H
#define Z_EN_FISHMOD_H

#include "ultra64.h"
#include "actor.h"
#include "fishmod.h"

#define EN_FISHMOD_MAX_LIMBS 16

// Texture roles: each is drawn from its own segment (0x08 + role) so species can swap in patterned textures.
typedef enum FishmodTexRole {
    /* 0 */ FISHMOD_TEX_ROLE_BACK,
    /* 1 */ FISHMOD_TEX_ROLE_BELLY,
    /* 2 */ FISHMOD_TEX_ROLE_HEAD,
    /* 3 */ FISHMOD_TEX_ROLE_SIDE,
    /* 4 */ FISHMOD_TEX_ROLE_FIN,
    /* 5 */ FISHMOD_TEX_ROLE_MAX
} FishmodTexRole;

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
} EnFishmod;

#endif
