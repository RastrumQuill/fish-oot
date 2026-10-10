/*
 * File: z_en_fishmod.c
 * Overlay: ovl_En_Fishmod
 * Description: fishmod overworld fishing. Type FISHMOD_TYPE_FISH is one fish of a species from
 * fishmod/data/species.json, drawn by reshaping and recoloring the Fishing Pond bass and loach models.
 * Type FISHMOD_TYPE_ROD is the per-scene fishing controller (rod, line, bobber, camera, spawning).
 */

#include "z_en_fishmod.h"

#include "assets/objects/object_fish/object_fish.h"
#include "assets/objects/gameplay_keep/gameplay_keep_0x18610.h"
#include "assets/objects/gameplay_keep/gFishInWaterAnim.h"
#include "array_count.h"
#include "libc64/math64.h"
#include "gfx.h"
#include "ultra64/libc.h"
#include "gfx_setupdl.h"
#include "ichain.h"
#include "rand.h"
#include "segmented_address.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "play_state.h"
#include "player.h"
#include "effect.h"
#include "save.h"
#include "sfx.h"
#include "bgcheck.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

void EnFishmod_Init(Actor* thisx, PlayState* play);
void EnFishmod_Destroy(Actor* thisx, PlayState* play);
void EnFishmod_Update(Actor* thisx, PlayState* play);
void EnFishmod_Draw(Actor* thisx, PlayState* play);

void EnFishmod_RodIdle(EnFishmod* this, PlayState* play);
void EnFishmod_FishHover(EnFishmod* this, PlayState* play) {
}

ActorProfile En_Fishmod_Profile = {
    /**/ ACTOR_EN_FISHMOD,
    /**/ ACTORCAT_NPC,
    /**/ FLAGS,
    /**/ OBJECT_FISH,
    /**/ sizeof(EnFishmod),
    /**/ EnFishmod_Init,
    /**/ EnFishmod_Destroy,
    /**/ EnFishmod_Update,
    /**/ EnFishmod_Draw,
};

/*
 * Which part of the fish each limb is (0-based limb index). Found with fishmod/tools/model_preview.py
 * and confirmed in game. FISHMOD_PART_NONE limbs are joints with little or no geometry.
 */
static u8 sBassLimbParts[] = {
    FISHMOD_PART_HEAD,         // 0
    FISHMOD_PART_NONE,         // 1  front body joint
    FISHMOD_PART_NONE,         // 2  rear body joint
    FISHMOD_PART_NONE,         // 3  tail joint
    FISHMOD_PART_TAIL,         // 4  tail fin
    FISHMOD_PART_TAIL,         // 5  tail fin center
    FISHMOD_PART_BODY_REAR,    // 6
    FISHMOD_PART_FIN_BELLY,    // 7  anal fin
    FISHMOD_PART_FIN_DORSAL,   // 8  soft dorsal fin
    FISHMOD_PART_BODY_FRONT,   // 9
    FISHMOD_PART_FIN_BELLY,    // 10 pelvic fin
    FISHMOD_PART_FIN_DORSAL,   // 11
    FISHMOD_PART_FIN_DORSAL,   // 12 spiny dorsal fin
    FISHMOD_PART_FIN_PECTORAL, // 13
    FISHMOD_PART_FIN_PECTORAL, // 14
};

static u8 sLoachLimbParts[] = {
    FISHMOD_PART_HEAD,         // 0
    FISHMOD_PART_BODY_FRONT,   // 1
    FISHMOD_PART_BODY_FRONT,   // 2
    FISHMOD_PART_FIN_DORSAL,   // 3
    FISHMOD_PART_TAIL,         // 4  tail fin
    FISHMOD_PART_BODY_REAR,    // 5  tail stalk
    FISHMOD_PART_BODY_REAR,    // 6
    FISHMOD_PART_BODY_FRONT,   // 7
    FISHMOD_PART_FIN_PECTORAL, // 8
    FISHMOD_PART_FIN_PECTORAL, // 9
    FISHMOD_PART_FIN_DORSAL,   // 10
};

static u8 sSmallLimbParts[] = {
    FISHMOD_PART_NONE,      // 0 root
    FISHMOD_PART_NONE,      // 1
    FISHMOD_PART_HEAD,      // 2 head and front body
    FISHMOD_PART_NONE,      // 3
    FISHMOD_PART_BODY_REAR, // 4
    FISHMOD_PART_TAIL,      // 5 tail fin
};

#include "fishmod_textures.inc.c"

// Pond textures for each role (back, belly, head, side, fin); must match BASE_TEXTURES in gen_textures.py
static void* sTextureRoles[3][FISHMOD_TEX_ROLE_MAX] = {
    // the bass has no side texture, so its "side" slot is the tail fin
    { gFishingFishTex_0016D0, gFishingFishTex_0018D0, gFishingFishTex_001AD0, gFishingFishTex_0014D0,
      gFishingFishTex_0022D0 },
    { gFishingLoachTex_00ED28, gFishingLoachTex_00F528, gFishingLoachTex_010528, gFishingLoachTex_00FD28,
      gFishingLoachTex_010DA8 },
    // small fish: body and tail fin
    { gFishBodyTex, NULL, NULL, gFishTailTex, NULL },
};

#define TEXSET_BASS 0
#define TEXSET_LOACH 1
#define TEXSET_SMALL 2

static s32 EnFishmod_TexSet(const FishmodSpecies* species) {
    if (species->base == FISHMOD_BASE_LOACH) {
        return TEXSET_LOACH;
    }
    if (species->base == FISHMOD_BASE_SMALL) {
        return TEXSET_SMALL;
    }
    return TEXSET_BASS;
}

static u8* EnFishmod_PartMap(const FishmodSpecies* species) {
    if (species->base == FISHMOD_BASE_LOACH) {
        return sLoachLimbParts;
    }
    if (species->base == FISHMOD_BASE_SMALL) {
        return sSmallLimbParts;
    }
    return sBassLimbParts;
}

/**
 * The pond models set their own primitive color (the tint) in every limb display list. Remove those
 * commands from the copy of object_fish in RAM so each fish can set its own tint per limb.
 * Idempotent, so it is safe to run for every fish.
 */
#if FISHMOD_OPT >= 2
/**
 * Turns the RGBA16 texture load starting at `g` (gsDPLoadTextureBlock: SETTIMG, SETTILE, LOADSYNC, LOADBLOCK,
 * PIPESYNC, SETTILE, SETTILESIZE) into a CI8 load with palette 0, and the palette mode set just before it
 * (gsDPSetTextureLUT) to RGBA16 palettes. The species' palette is loaded before each fish is drawn.
 */
static void EnFishmod_LoadToCI8(Gfx* g, Gfx* dlStart) {
    u32 w;
    u32 h;
    u32 words;

    if (((g[1].words.w0 >> 24) != G_SETTILE) || ((g[3].words.w0 >> 24) != G_LOADBLOCK) ||
        ((g[5].words.w0 >> 24) != G_SETTILE) || ((g[6].words.w0 >> 24) != G_SETTILESIZE)) {
        return;
    }
    w = ((g[6].words.w1 >> 12) & 0xFFF) / 4 + 1;
    h = (g[6].words.w1 & 0xFFF) / 4 + 1;
    words = (w + 7) / 8; // 64-bit words per row at 8 bits per texel
    if (words == 0) {
        words = 1;
    }

    g[0].words.w0 = (g[0].words.w0 & ~(7 << 21)) | (G_IM_FMT_CI << 21);
    g[1].words.w0 = (g[1].words.w0 & ~(7 << 21)) | (G_IM_FMT_CI << 21);
    g[3].words.w1 = (g[3].words.w1 & 0xFF000000) | (((((w * h) + 1) >> 1) - 1) << 12) |
                    (((1 << G_TX_DXT_FRAC) + words - 1) / words);
    g[5].words.w0 = (G_SETTILE << 24) | (G_IM_FMT_CI << 21) | (G_IM_SIZ_8b << 19) | (((w + 7) >> 3) << 9) |
                    (g[5].words.w0 & 0x1FF);
    g[5].words.w1 &= ~(0xF << 20);

    // gsDPSetTextureLUT(G_TT_NONE) just before the load
    if ((g > dlStart) && ((g[-1].words.w0 >> 24) == G_SETOTHERMODE_H) &&
        (((g[-1].words.w0 >> 8) & 0xFF) == (32 - G_MDSFT_TEXTLUT - 2))) {
        g[-1].words.w1 = G_TT_RGBA16;
    }
}
#endif

static void EnFishmod_PatchSkeleton(FlexSkeletonHeader* skelHeader, s32 texSet) {
    void** limbs = SEGMENTED_TO_VIRTUAL(skelHeader->sh.segment);
    s32 i;
    s32 role;

    for (i = 0; i < skelHeader->sh.limbCount; i++) {
        StandardLimb* limb = SEGMENTED_TO_VIRTUAL(limbs[i]);
        Gfx* gfx;
        s32 n;

        if (limb->dList == NULL) {
            continue;
        }

        gfx = SEGMENTED_TO_VIRTUAL(limb->dList);
        for (n = 0; n < 200; n++, gfx++) {
            u8 opcode = gfx->words.w0 >> 24;

            if (opcode == G_ENDDL) {
                break;
            }
            if (opcode == G_SETPRIMCOLOR) {
                gfx->words.w0 = G_NOOP << 24;
                gfx->words.w1 = 0;
            } else if (opcode == G_SETTIMG) {
                // load role textures from their segments instead
                for (role = 0; role < FISHMOD_TEX_ROLE_MAX; role++) {
                    if ((sTextureRoles[texSet][role] != NULL) &&
                        (gfx->words.w1 == (uintptr_t)sTextureRoles[texSet][role])) {
                        gfx->words.w1 = (u32)(0x08 + role) << 24;
#if FISHMOD_OPT >= 2
                        EnFishmod_LoadToCI8(gfx, SEGMENTED_TO_VIRTUAL(limb->dList));
#endif
                        break;
                    }
                }
            }
        }
    }
}

/*
 * The small fish lives in gameplay_keep, which is shared with the regular bottle fish, so its display lists
 * are copied into the overlay and only the copies are patched (tint removed, textures moved to segments).
 */
#define SMALL_DL_MAX 40
static Gfx sSmallDLs[3][SMALL_DL_MAX];
static Gfx* sSmallDLSources[3] = { gameplay_keep_018A68_DL, gameplay_keep_018950_DL, gameplay_keep_0188B0_DL };
static u8 sSmallDLsReady = false;

static void EnFishmod_PrepareSmallDLs(void) {
    s32 i;
    s32 n;
    s32 role;

    if (sSmallDLsReady) {
        return;
    }
    for (i = 0; i < 3; i++) {
        Gfx* src = SEGMENTED_TO_VIRTUAL(sSmallDLSources[i]);
        Gfx* dst = sSmallDLs[i];

        for (n = 0; n < SMALL_DL_MAX; n++) {
            u8 opcode = src[n].words.w0 >> 24;

            dst[n] = src[n];
            if (opcode == G_ENDDL) {
                break;
            }
            if (opcode == G_SETPRIMCOLOR) {
                dst[n].words.w0 = G_NOOP << 24;
                dst[n].words.w1 = 0;
            } else if (opcode == G_SETTIMG) {
                for (role = 0; role < FISHMOD_TEX_ROLE_MAX; role++) {
                    if ((sTextureRoles[TEXSET_SMALL][role] != NULL) &&
                        (dst[n].words.w1 == (uintptr_t)sTextureRoles[TEXSET_SMALL][role])) {
                        dst[n].words.w1 = (u32)(0x08 + role) << 24;
#if FISHMOD_OPT >= 2
                        // the rest of the load block hasn't been copied yet
                        {
                            s32 k;

                            for (k = 1; k <= 6; k++) {
                                dst[n + k] = src[n + k];
                            }
                            EnFishmod_LoadToCI8(&dst[n], dst);
                            n += 6; // keep the converted commands
                        }
#endif
                        break;
                    }
                }
            }
        }
    }
    sSmallDLsReady = true;
}

typedef enum EnFishmodLimbMode {
    LIMB_MODE_PLAIN,       // no part scaling to apply here
    LIMB_MODE_OWN_SCALE,   // scaled, parent isn't: translate/rotate, then scale
    LIMB_MODE_UNDO_PARENT, // not scaled, parent is: undo the parent's scale, let the skeleton translate/rotate
    LIMB_MODE_FULL         // both
} EnFishmodLimbMode;

static void EnFishmod_InitLimbData(EnFishmod* this, FlexSkeletonHeader* skelHeader) {
    void** limbs = SEGMENTED_TO_VIRTUAL(skelHeader->sh.segment);
    u8* partMap = EnFishmod_PartMap(this->species);
    s32 i;

    this->limbCount = skelHeader->sh.limbCount;
    if (this->limbCount > EN_FISHMOD_MAX_LIMBS) {
        this->limbCount = EN_FISHMOD_MAX_LIMBS;
    }

    for (i = 0; i < this->limbCount; i++) {
        this->limbParent[i] = 0xFF;
    }

    for (i = 0; i < this->limbCount; i++) {
        StandardLimb* limb = SEGMENTED_TO_VIRTUAL(limbs[i]);
        s32 child = limb->child;

        // every limb in the child's sibling chain has this limb as its parent
        while ((child != LIMB_DONE) && (child < this->limbCount)) {
            this->limbParent[child] = i;
            child = ((StandardLimb*)SEGMENTED_TO_VIRTUAL(limbs[child]))->sibling;
        }
    }

    for (i = 0; i < this->limbCount; i++) {
        u8 part = partMap[i];

        if (part == FISHMOD_PART_NONE) {
            this->limbScale[i].x = this->limbScale[i].y = this->limbScale[i].z = 1.0f;
        } else {
            this->limbScale[i].x = this->species->partScale[part][0] * 0.01f;
            this->limbScale[i].y = this->species->partScale[part][1] * 0.01f;
            this->limbScale[i].z = this->species->partScale[part][2] * 0.01f;
        }
    }

    for (i = 0; i < this->limbCount; i++) {
        Vec3f* s = &this->limbScale[i];
        Vec3f one = { 1.0f, 1.0f, 1.0f };
        Vec3f* p = (this->limbParent[i] != 0xFF) ? &this->limbScale[this->limbParent[i]] : &one;
        s32 ownScaled = (s->x != 1.0f) || (s->y != 1.0f) || (s->z != 1.0f);
        s32 parentScaled = (p->x != 1.0f) || (p->y != 1.0f) || (p->z != 1.0f);

        if (!ownScaled && !parentScaled) {
            this->limbMode[i] = LIMB_MODE_PLAIN;
#if FISHMOD_OPT >= 1
        } else if ((s->x == p->x) && (s->y == p->y) && (s->z == p->z) && (s->x == s->y) && (s->y == s->z)) {
            // the same uniform scale as the parent: the plain path already gives the same matrix
            this->limbMode[i] = LIMB_MODE_PLAIN;
        } else if (!parentScaled) {
            this->limbMode[i] = LIMB_MODE_OWN_SCALE;
        } else if (!ownScaled) {
            this->limbMode[i] = LIMB_MODE_UNDO_PARENT;
#endif
        } else {
            this->limbMode[i] = LIMB_MODE_FULL;
        }
    }
}

void EnFishmod_SetLength(EnFishmod* this, f32 lengthInches) {
    f32 cm = lengthInches * 2.54f;

    this->lengthInches = lengthInches;
    if (this->species->base == FISHMOD_BASE_LOACH) {
        Actor_SetScale(&this->actor, cm * 65.0f * 0.000001f);
        this->actor.scale.x = this->actor.scale.y = this->actor.scale.z * 1.1f;
    } else if (this->species->base == FISHMOD_BASE_SMALL) {
        // the small model is about 1300 units long; match the bass at about 1.6 world units per inch
        Actor_SetScale(&this->actor, lengthInches * 1.64f / 1300.0f);
    } else {
        Actor_SetScale(&this->actor, cm * 15.0f * 0.00001f);
    }

    // all the models come out at about 1.6 world units per inch; bodies are about a quarter as deep as long
    // (the loach is a slim eel)
    this->halfLength = lengthInches * 0.85f;
    this->halfHeight = lengthInches * ((this->species->base == FISHMOD_BASE_LOACH) ? 0.12f : 0.22f);
    this->halfHeight = CLAMP_MIN(this->halfHeight, 1.5f);
}

void EnFishmod_Init(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;
    FlexSkeletonHeader* skel;
    AnimationHeader* anim;
    s32 species;

    this->type = FISHMOD_GET_TYPE(thisx->params);

    if (this->type == FISHMOD_TYPE_ROD) {
        this->actionFunc = EnFishmod_RodIdle;
        EnFishmod_RodInit(this, play);
        return;
    }

    species = FISHMOD_GET_SPECIES(thisx->params);
    if (species >= FISHMOD_SPECIES_MAX) {
        species = FISHMOD_SPECIES_GREENGILL;
    }
    this->species = &gFishmodSpecies[species];

    if (this->species->base == FISHMOD_BASE_LOACH) {
        skel = &gFishingLoachSkel;
        anim = &gFishingLoachAnim;
    } else if (this->species->base == FISHMOD_BASE_SMALL) {
        skel = &gFishSkel;
        anim = &gFishInWaterAnim;
    } else {
        skel = &gFishingFishSkel;
        anim = &gFishingFishAnim;
    }

    SkelAnime_InitFlex(play, &this->skelAnime, skel, anim, this->jointTable, this->morphTable,
                       EN_FISHMOD_MAX_LIMBS + 1);
    Animation_MorphToLoop(&this->skelAnime, anim, 0.0f);

    if (this->species->base == FISHMOD_BASE_SMALL) {
        EnFishmod_PrepareSmallDLs();
    } else {
        EnFishmod_PatchSkeleton(SEGMENTED_TO_VIRTUAL(skel), EnFishmod_TexSet(this->species));
    }
    EnFishmod_InitLimbData(this, SEGMENTED_TO_VIRTUAL(skel));

    if (gFishmodPhoto.active) {
        EnFishmod_SetLength(this, this->species->lengthTypical);
        this->fishState = FISH_STATE_PHOTO;
    } else {
        // length: triangular distribution between min and max, peaking at the typical size
        f32 lo = this->species->lengthMin;
        f32 hi = this->species->lengthMax;
        f32 mode = (this->species->lengthTypical - lo) / (hi - lo);
        f32 u = Rand_ZeroOne();
        f32 f = (u < mode) ? sqrtf(u * mode) : 1.0f - sqrtf((1.0f - u) * (1.0f - mode));

        EnFishmod_SetLength(this, lo + (hi - lo) * f);
        this->fishState = FISH_STATE_WANDER;
        this->stateTimer = 0;
        // don't draw fish that are off screen or far away (they still swim)
        this->actor.flags &= ~ACTOR_FLAG_DRAW_CULLING_DISABLED;
        this->actor.cullingVolumeDistance = 900.0f;
        this->actor.cullingVolumeScale = this->halfLength * 2.0f + 20.0f;
        this->actor.cullingVolumeDownward = this->halfLength * 2.0f + 20.0f;
    }
    this->staminaMax = 60.0f + this->species->behavior.stamina * 3.0f + this->lengthInches * 4.0f;
    this->stamina = this->staminaMax;
    this->swimPhaseStep = 0.25f;
    this->swimMag = 1500.0f;
    this->swimPhase = Rand_ZeroFloat(6.28f);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    this->actionFunc = EnFishmod_FishHover;
}

void EnFishmod_Destroy(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;

    if (this->type == FISHMOD_TYPE_ROD) {
        EnFishmod_RodDestroy(this, play);
        if (gFishmodRod == thisx) {
            gFishmodRod = NULL;
        }
    } else {
        EnFishmod_RodRelease(this);
        SkelAnime_Free(&this->skelAnime, play);
    }
}

void EnFishmod_RodIdle(EnFishmod* this, PlayState* play) {
}

/* ---------------------------------------------------------------------------------------------------------------
 * Fish AI
 */

s32 EnFishmod_FindWaterPoint(PlayState* play, Vec3f* center, f32 radius, f32 minDepth, Vec3f* out, f32* outSurface,
                             f32* outFloor) {
    s32 tries;

    for (tries = 0; tries < 16; tries++) {
        Vec3f probe;
        f32 surface;
        f32 floor;

        probe.x = center->x + Rand_CenteredFloat(2.0f * radius);
        probe.z = center->z + Rand_CenteredFloat(2.0f * radius);
        if (!EnFishmod_WaterFloor(play, probe.x, probe.z, &surface, &floor, true)) {
            continue;
        }
        if ((floor <= BGCHECK_Y_MIN) || ((surface - floor) < minDepth)) {
            continue;
        }
        out->x = probe.x;
        out->z = probe.z;
        out->y = floor + (surface - floor) * (0.3f + Rand_ZeroFloat(0.4f));
        *outSurface = surface;
        *outFloor = floor;
        return true;
    }
    return false;
}

/** Water surface and floor at a point (from the cache, erring high). Returns false if there's no water there. */
static s32 EnFishmod_WaterAt(PlayState* play, f32 x, f32 z, f32* surface, f32* floor) {
    return EnFishmod_WaterFloor(play, x, z, surface, floor, true);
}

/** Water depth the whole body needs: its height, more when it is pitched up or down. */
static f32 EnFishmod_BodyClearance(EnFishmod* this) {
    return this->halfHeight + ABS(Math_SinS(this->actor.shape.rot.x)) * this->halfLength + 1.0f;
}

/**
 * Updates waterY/floorY for a fish centered at pos and facing yaw: the floor is the highest under its center
 * and its nose, so it turns away from a bank before its head goes in. Returns false if it doesn't fit there.
 */
static s32 EnFishmod_ProbeWater(EnFishmod* this, PlayState* play, Vec3f* pos, s16 yaw) {
    f32 surface;
    f32 floor;
    f32 noseSurface;
    f32 noseFloor;

    if (!EnFishmod_WaterAt(play, pos->x, pos->z, &surface, &floor) ||
        !EnFishmod_WaterAt(play, pos->x + Math_SinS(yaw) * this->halfLength,
                           pos->z + Math_CosS(yaw) * this->halfLength, &noseSurface, &noseFloor)) {
        return false;
    }
    this->waterY = CLAMP_MAX(surface, noseSurface);
    this->floorY = CLAMP_MIN(floor, noseFloor);
    return (this->waterY - this->floorY) > (this->halfHeight * 2.0f + 2.0f);
}

/**
 * Water depth where the fish is now, if that is too shallow for it (stranded), or a depth no probe will beat if
 * it is swimming freely.
 */
static f32 EnFishmod_ShallowDepth(EnFishmod* this, PlayState* play) {
    f32 surface;
    f32 floor;

    if (EnFishmod_WaterAt(play, this->actor.world.pos.x, this->actor.world.pos.z, &surface, &floor) &&
        ((surface - floor) < this->halfHeight * 2.0f + 2.0f)) {
        return (surface - floor) + 0.5f;
    }
    return 100000.0f;
}

/** Keeps the body between the floor and the surface. */
static f32 EnFishmod_ClampY(EnFishmod* this, f32 y) {
    f32 clearance = EnFishmod_BodyClearance(this);
    f32 lo = this->floorY + clearance;
    f32 hi = this->waterY - clearance;

    if (lo > hi) {
        return (this->floorY + this->waterY) * 0.5f;
    }
    return CLAMP(y, lo, hi);
}

static f32 EnFishmod_DepthY(EnFishmod* this, f32 surface, f32 floor) {
    f32 depth = surface - floor;

    switch (this->species->depth) {
        case FISHMOD_DEPTH_SURFACE:
            return surface - CLAMP_MAX(6.0f, depth * 0.5f);
        case FISHMOD_DEPTH_BOTTOM:
            return floor + CLAMP_MAX(5.0f, depth * 0.4f);
        default:
            return floor + depth * 0.5f;
    }
}

/** How appealing the bobber is right now: bait, time of day and twitches. */
static f32 EnFishmod_BaitAppeal(EnFishmod* this) {
    f32 appeal = this->species->bait[gFishmodRodState.bait] * 0.01f;
    u16 time = gSaveContext.save.dayTime;
    s32 tod;

    if ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(8, 0))) {
        tod = FISHMOD_TIME_DAWN;
    } else if ((time >= CLOCK_TIME(8, 0)) && (time < CLOCK_TIME(17, 0))) {
        tod = FISHMOD_TIME_DAY;
    } else if ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(20, 0))) {
        tod = FISHMOD_TIME_DUSK;
    } else {
        tod = FISHMOD_TIME_NIGHT;
    }
    appeal *= (this->species->activeTimes & (1 << tod)) ? 1.3f : 0.6f;
    if (gFishmodRodState.twitchTimer > 0) {
        appeal *= 1.5f;
    }
    return appeal;
}

static void EnFishmod_PickWanderTarget(EnFishmod* this, PlayState* play) {
    Vec3f point;
    f32 surface;
    f32 floor;

    if (EnFishmod_FindWaterPoint(play, &this->actor.home.pos, 130.0f,
                                 CLAMP_MIN(12.0f, this->halfHeight * 2.0f + this->halfLength * 0.5f), &point,
                                 &surface, &floor)) {
        point.y = EnFishmod_DepthY(this, surface, floor);
        this->target = point;
    } else {
        this->target = this->actor.home.pos;
    }
    this->stateTimer = 60 + (s16)Rand_ZeroFloat(120.0f);
}

void EnFishmod_FishSetState(EnFishmod* this, s32 state) {
    if ((state != FISH_STATE_HOOKED) && (state != FISH_STATE_CAUGHT) && (state != FISH_STATE_NIBBLE) &&
        (state != FISH_STATE_BITE) && (state != FISH_STATE_INSPECT)) {
        EnFishmod_RodRelease(this);
    }

    this->fishState = state;
    switch (state) {
        case FISH_STATE_WANDER:
            this->stateTimer = 0;
            break;
        case FISH_STATE_INSPECT:
            this->stateTimer = 30 + (s16)Rand_ZeroFloat(60.0f);
            break;
        case FISH_STATE_NIBBLE:
            this->stateTimer = 12;
            break;
        case FISH_STATE_HOOKED:
            this->stamina = this->staminaMax;
            this->runTimer = 0;
            this->jumping = false;
            this->runYaw = this->actor.shape.rot.y;
            this->beached = false;
            break;
        case FISH_STATE_FLEE:
            this->stateTimer = 50;
            this->cooldown = 300 + (s16)Rand_ZeroFloat(300.0f);
            this->jumping = false;
            break;
    }
}

/**
 * A direction with open water a body length or two ahead, trying `preferred` first, then turning a little more
 * either way each try. Falls back to the way home.
 */
static s16 EnFishmod_OpenWaterYaw(EnFishmod* this, PlayState* play, s16 preferred) {
    static s16 sTurns[] = { 0, 0x1800, -0x1800, 0x3000, -0x3000, 0x4800, -0x4800, 0x6000, -0x6000, 0x8000 };
    f32 reach = this->halfLength * 2.0f + 15.0f;
    f32 savedWater = this->waterY;
    f32 savedFloor = this->floorY;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sTurns); i++) {
        s16 yaw = preferred + sTurns[i];
        Vec3f probe;

        probe.x = this->actor.world.pos.x + Math_SinS(yaw) * reach;
        probe.y = this->actor.world.pos.y;
        probe.z = this->actor.world.pos.z + Math_CosS(yaw) * reach;
        if (EnFishmod_ProbeWater(this, play, &probe, yaw)) {
            this->waterY = savedWater;
            this->floorY = savedFloor;
            return yaw;
        }
    }
    this->waterY = savedWater;
    this->floorY = savedFloor;
    return Math_Vec3f_Yaw(&this->actor.world.pos, &this->actor.home.pos);
}

/** Steers toward target at the given speed, staying inside the water. Returns false if the shore blocked it. */
static s32 EnFishmod_SwimToward(EnFishmod* this, PlayState* play, Vec3f* target, f32 speed, s16 turnStep) {
    Vec3f next;
    f32 surface;
    f32 floor;
    s16 yaw = Math_Vec3f_Yaw(&this->actor.world.pos, target);
    f32 dy = target->y - this->actor.world.pos.y;
    f32 distXZ = Math_Vec3f_DistXZ(&this->actor.world.pos, target);
    s32 moved = true;

    if (distXZ < 2.0f + this->halfLength * 0.3f) {
        // Basically there: hold the heading and drift onto the spot. Turning toward a point this close would
        // swing the fish around wildly with every tiny move.
        f32 step = CLAMP_MAX(distXZ * 0.2f, speed);

        Math_ApproachF(&this->speed, 0.0f, 1.0f, 0.08f);
        next.x = this->actor.world.pos.x;
        next.z = this->actor.world.pos.z;
        if (distXZ > 0.01f) {
            next.x += (target->x - this->actor.world.pos.x) / distXZ * step;
            next.z += (target->z - this->actor.world.pos.z) / distXZ * step;
        }
    } else {
        // working around a bank: follow the detour for a while before aiming at the target again
        if (this->detourTimer > 0) {
            this->detourTimer--;
            yaw = this->detourYaw;
            turnStep = CLAMP_MIN(turnStep, 0x400);
        }
        Math_SmoothStepToS(&this->actor.shape.rot.y, yaw, 4, turnStep, 0x40);
        Math_ApproachF(&this->speed, speed, 1.0f, 0.08f);
        next.x = this->actor.world.pos.x + Math_SinS(this->actor.shape.rot.y) * this->speed;
        next.z = this->actor.world.pos.z + Math_CosS(this->actor.shape.rot.y) * this->speed;
    }
    next.y = this->actor.world.pos.y + CLAMP(dy * 0.08f, -0.8f, 0.8f);

    if (EnFishmod_ProbeWater(this, play, &next, this->actor.shape.rot.y)) {
        next.y = EnFishmod_ClampY(this, next.y);
        this->actor.world.pos = next;
    } else if (EnFishmod_WaterAt(play, next.x, next.z, &surface, &floor) &&
               (((surface - floor) > EnFishmod_ShallowDepth(this, play)) ||
                // following a detour out of a tight spot: water a little shallow is fine, land isn't
                ((this->detourTimer > 0) && ((surface - floor) > this->halfHeight * 1.2f)))) {
        // stranded somewhere too shallow (released or dragged there): anywhere deeper is better
        this->waterY = surface;
        this->floorY = floor;
        next.y = EnFishmod_ClampY(this, next.y);
        this->actor.world.pos = next;
    } else {
        // shore ahead: head for open water instead, and keep going that way for a bit so it can get around
        s16 open = EnFishmod_OpenWaterYaw(this, play, yaw);

        this->detourYaw = open;
        this->detourTimer = 20;

        this->target.x = this->actor.world.pos.x + Math_SinS(open) * (this->halfLength * 2.0f + 30.0f);
        this->target.z = this->actor.world.pos.z + Math_CosS(open) * (this->halfLength * 2.0f + 30.0f);
        this->target.y = this->actor.world.pos.y;
        Math_SmoothStepToS(&this->actor.shape.rot.y, open, 2, 0x800, 0x100);
        this->speed *= 0.5f;
        moved = false;
    }

    this->actor.shape.rot.x = CLAMP(-(s16)(dy * 300.0f), -0x1800, 0x1800);
    this->blocked = !moved;
    return moved;
}

static void EnFishmod_CheckSpook(EnFishmod* this, PlayState* play) {
    FishmodRod* rod = &gFishmodRodState;
    Player* player = GET_PLAYER(play);
    f32 warySq = 30.0f + this->species->behavior.wariness * 0.7f;

    if (rod->splashed && (Math_Vec3f_DistXZ(&this->actor.world.pos, &rod->bobberPos) < warySq)) {
        EnFishmod_FishSetState(this, FISH_STATE_FLEE);
        return;
    }
    // Link wading in close scares wary fish
    if ((player->actor.depthInWater > 0.0f) &&
        (Math_Vec3f_DistXZ(&this->actor.world.pos, &player->actor.world.pos) < this->species->behavior.wariness * 0.8f)) {
        EnFishmod_FishSetState(this, FISH_STATE_FLEE);
    }
}

/** True if there's water deep enough for this fish all the way from it to `to` (it can see and reach it). */
static s32 EnFishmod_OpenWaterTo(EnFishmod* this, PlayState* play, Vec3f* to) {
    s32 i;

    // up to just short of the target (the fish stops a body length back), with the interpolated floor: the
    // cautious one reads the water next to a steep bank as dry
    for (i = 1; i <= 5; i++) {
        f32 t = i / 6.0f;
        f32 surface;
        f32 floor;

        if (!EnFishmod_WaterFloor(play, this->actor.world.pos.x + (to->x - this->actor.world.pos.x) * t,
                                  this->actor.world.pos.z + (to->z - this->actor.world.pos.z) * t, &surface, &floor,
                                  false) ||
            ((surface - floor) < this->halfHeight * 1.5f)) {
            return false;
        }
    }
    return true;
}

/**
 * Where the fish's center goes so its mouth is `reach` body-halves short of the bobber, along the heading it
 * came in on (runYaw, set when it starts inspecting). Keeping one heading under the bobber stops it from
 * swinging around to face the bobber every time either of them moves a little.
 */
static void EnFishmod_BobberStation(EnFishmod* this, f32 reach, f32 y, Vec3f* out) {
    FishmodRod* rod = &gFishmodRodState;

    out->x = rod->bobberPos.x - Math_SinS(this->runYaw) * this->halfLength * reach;
    out->z = rod->bobberPos.z - Math_CosS(this->runYaw) * this->halfLength * reach;
    out->y = y;
}

void EnFishmod_FishAI(EnFishmod* this, PlayState* play) {
    FishmodRod* rod = &gFishmodRodState;
    const FishmodBehavior* b = &this->species->behavior;
    f32 bobberDist = Math_Vec3f_DistXZ(&this->actor.world.pos, &rod->bobberPos);
    Vec3f below;

    if (this->cooldown > 0) {
        this->cooldown--;
    }
    if (this->stateTimer > 0) {
        this->stateTimer--;
    }

    below = rod->bobberPos;
    below.y = rod->waterY - 10.0f;

    switch (this->fishState) {
        case FISH_STATE_WANDER:
            if ((this->stateTimer == 0) || (Math_Vec3f_DistXYZ(&this->actor.world.pos, &this->target) < 8.0f)) {
                EnFishmod_PickWanderTarget(this, play);
            }
            EnFishmod_SwimToward(this, play, &this->target, 0.4f + b->speed * 0.008f, 0x200);
            this->swimPhaseStep = 0.15f + this->speed * 0.12f;
            this->swimMag = 900.0f;

            EnFishmod_CheckSpook(this, play);
            if ((this->fishState == FISH_STATE_WANDER) && (this->cooldown == 0) && EnFishmod_RodBobberInWater() &&
                (((play->gameplayFrames + this->actor.params) % 10) == 0)) {
                f32 notice = 45.0f + b->curiosity * 1.4f;

                if (rod->twitchTimer > 0) {
                    notice += 40.0f;
                }
                if ((bobberDist < notice) && (Rand_ZeroOne() < b->curiosity * 0.004f * EnFishmod_BaitAppeal(this)) &&
                    EnFishmod_OpenWaterTo(this, play, &rod->bobberPos)) {
                    this->fishState = FISH_STATE_APPROACH;
                    this->stateTimer = 200;
                }
            }
            break;

        case FISH_STATE_APPROACH:
            if (!EnFishmod_RodBobberInWater() || (this->stateTimer == 0)) {
                // gave up (couldn't get there): don't come straight back
                this->cooldown = 150;
                EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                break;
            }
            {
                Vec3f station;

                // come in nose first: aim for the spot a body length short of the bobber
                this->runYaw = Math_Vec3f_Yaw(&this->actor.world.pos, &rod->bobberPos);
                EnFishmod_BobberStation(this, 0.9f, below.y, &station);
                EnFishmod_SwimToward(this, play, &station, 0.7f + b->speed * 0.01f, 0x400);
            }
            this->swimPhaseStep = 0.2f + this->speed * 0.15f;
            EnFishmod_CheckSpook(this, play);
            if ((this->fishState == FISH_STATE_APPROACH) && (bobberDist < 14.0f + this->halfLength)) {
                if ((rod->fish == NULL) || (rod->fish == this)) {
                    rod->fish = this;
                    this->nibblesLeft = b->nibbles;
                    this->interest = 0.0f;
                    EnFishmod_FishSetState(this, FISH_STATE_INSPECT);
                } else {
                    // someone else got there first: hang around a bit
                    this->cooldown = 60;
                    EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                }
            }
            break;

        case FISH_STATE_INSPECT: {
            Vec3f hover = below;

            if (!EnFishmod_RodBobberInWater() || (rod->fish != this)) {
                EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                break;
            }
            // hold station under the bobber, nose toward it, swaying a little
            EnFishmod_BobberStation(this, 0.9f, below.y, &hover);
            hover.x += Math_CosS(this->runYaw) * Math_SinS(this->timer * 0x200) * 2.0f;
            hover.z -= Math_SinS(this->runYaw) * Math_SinS(this->timer * 0x200) * 2.0f;
            EnFishmod_SwimToward(this, play, &hover, 0.25f, 0x200);
            Math_SmoothStepToS(&this->actor.shape.rot.y, this->runYaw, 8, 0x100, 0x20);
            this->swimPhaseStep = 0.12f;

            this->interest += 0.01f * EnFishmod_BaitAppeal(this);
            // reeling the bobber away too fast is suspicious to wary fish
            // (the bobber has moved away from the spot the fish is holding)
            if ((Math_Vec3f_DistXZ(&this->actor.world.pos, &hover) > 12.0f) &&
                (Rand_ZeroOne() < b->wariness * 0.002f)) {
                this->cooldown = 200;
                EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                break;
            }
            if (this->stateTimer == 0) {
                if (this->nibblesLeft > 0) {
                    EnFishmod_FishSetState(this, FISH_STATE_NIBBLE);
                } else if (Rand_ZeroOne() < (b->aggression * 0.012f * (0.6f + this->interest))) {
                    this->fishState = FISH_STATE_BITE;
                    this->stateTimer = 0;
                    EnFishmod_RodBite(this);
                } else {
                    // lost interest
                    this->cooldown = 240;
                    EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                }
            }
            break;
        }

        case FISH_STATE_NIBBLE: {
            Vec3f peck = rod->bobberPos;

            if (!EnFishmod_RodBobberInWater() || (rod->fish != this)) {
                EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                break;
            }
            // dart forward to peck, then back off, without turning
            if (this->stateTimer > 6) {
                EnFishmod_BobberStation(this, 0.45f, rod->waterY - 4.0f, &peck);
            } else {
                EnFishmod_BobberStation(this, 0.9f, below.y, &peck);
            }
            EnFishmod_SwimToward(this, play, &peck, 0.8f, 0x200);
            Math_SmoothStepToS(&this->actor.shape.rot.y, this->runYaw, 8, 0x100, 0x20);
            this->swimPhaseStep = 0.35f;
            this->jawRot = (this->stateTimer > 4) ? 4000 : 0;
            if (this->stateTimer == 6) {
                EnFishmod_RodNibble(this);
            }
            if (this->stateTimer == 0) {
                this->nibblesLeft--;
                this->jawRot = 0;
                this->fishState = FISH_STATE_INSPECT;
                this->stateTimer = 15 + (s16)Rand_ZeroFloat(35.0f);
            }
            break;
        }

        case FISH_STATE_BITE: {
            Vec3f mouth = rod->bobberPos;

            if (!EnFishmod_RodBobberInWater() || (rod->fish != this)) {
                EnFishmod_FishSetState(this, FISH_STATE_WANDER);
                break;
            }
            // takes the bait and pulls the bobber down
            EnFishmod_BobberStation(this, 0.5f, rod->bobberPos.y - 4.0f, &mouth);
            EnFishmod_SwimToward(this, play, &mouth, 1.0f, 0x200);
            Math_SmoothStepToS(&this->actor.shape.rot.y, this->runYaw, 8, 0x100, 0x20);
            this->actor.shape.rot.x = 0x1000;
            this->swimPhaseStep = 0.45f;
            this->jawRot = 6000;
            break;
        }

        case FISH_STATE_HOOKED: {
            Player* player = GET_PLAYER(play);
            Vec3f next;

            this->jawRot = 3000;
            // Now and then it makes a run: away from Link, veering left or right, toward open water, with a burst
            // of speed at the start (stateTimer counts the burst down; the rod pulls harder while it lasts).
            if (--this->runTimer <= 0) {
                this->runYaw = EnFishmod_OpenWaterYaw(this, play,
                                                      Math_Vec3f_Yaw(&player->actor.world.pos, &this->actor.world.pos) +
                                                          (s16)Rand_CenteredFloat(0x7000));
                this->runTimer = 40 + (s16)Rand_ZeroFloat(70.0f);
                this->stateTimer = (s16)((12.0f + b->strength * 0.3f) * (0.3f + 0.7f * this->stamina / this->staminaMax));
            }

            // rod has computed where the line lets it go
            next = this->target;
            if (this->jumping) {
                f32 surface;
                f32 floor;

                if (!EnFishmod_WaterAt(play, next.x, next.z, &surface, &floor) || (floor > surface - 4.0f)) {
                    // don't leap out over the bank
                    next.x = this->actor.world.pos.x;
                    next.z = this->actor.world.pos.z;
                }
                this->vel.y -= 0.7f;
                next.y = this->actor.world.pos.y + this->vel.y;
                if (next.y < this->waterY - 2.0f) {
                    this->jumping = false;
                    next.y = EnFishmod_ClampY(this, this->waterY - 4.0f);
                    {
                        Vec3f splash = next;

                        splash.y = this->waterY;
                        EffectSsGSplash_Spawn(play, &splash, NULL, NULL, 0, 250);
                        EffectSsGRipple_Spawn(play, &splash, 80, 400, 0);
                    }
                    Actor_PlaySfx(&this->actor, NA_SE_EV_DIVE_INTO_WATER_L);
                }
                this->actor.shape.rot.x = -(s16)(this->vel.y * 1500.0f);
                this->actor.world.pos = next;
            } else {
                f32 surface;
                f32 floor;
                s32 fits = EnFishmod_ProbeWater(this, play, &next, this->runYaw);
                // the line drags it toward Link, even through water too shallow to swim in
                s32 reeledIn = Math_Vec3f_DistXZ(&next, &player->actor.world.pos) <
                               Math_Vec3f_DistXZ(&this->actor.world.pos, &player->actor.world.pos);

                if (fits) {
                    next.y = this->actor.world.pos.y;
                    Math_ApproachF(&next.y, this->waterY - 8.0f, 0.1f, 1.0f);
                    next.y = EnFishmod_ClampY(this, next.y);
                    this->actor.world.pos = next;
                    this->actor.shape.rot.x = 0;

                    // leap
                    if ((gFishmodRodState.tension > 45.0f) && (this->stamina > this->staminaMax * 0.2f) &&
                        (Rand_ZeroOne() < b->jumpiness * 0.0004f)) {
                        Vec3f splash = this->actor.world.pos;

                        this->jumping = true;
                        this->vel.y = 6.0f + b->strength * 0.04f;
                        splash.y = this->waterY;
                        EffectSsGSplash_Spawn(play, &splash, NULL, NULL, 0, 300);
                        Actor_PlaySfx(&this->actor, NA_SE_EV_FISH_LEAP);
                    }
                } else if (reeledIn && EnFishmod_WaterAt(play, next.x, next.z, &surface, &floor) &&
                           (surface - floor > 1.0f)) {
                    // thrashing in the shallows, belly on the bottom
                    this->waterY = surface;
                    this->floorY = floor;
                    next.y = CLAMP_MIN(floor + this->halfHeight, (floor + surface) * 0.5f);
                    this->actor.world.pos = next;
                    this->actor.shape.rot.x = 0;
                } else if (reeledIn) {
                    this->beached = true;
                } else {
                    // can't go that way: shore. Turn for open water.
                    this->runYaw = EnFishmod_OpenWaterYaw(this, play, this->runYaw + 0x4000);
                    this->runTimer = 20;
                }
            }

            if (this->stateTimer > 0) {
                // running: hard, fast tail beats
                Math_SmoothStepToS(&this->actor.shape.rot.y, this->runYaw, 3, 0x1000, 0x100);
                this->swimPhaseStep = 0.55f + (this->stamina / this->staminaMax) * 0.3f;
                this->swimMag = 2600.0f;
                this->actor.shape.rot.z = 0;
            } else {
                // holding against the line: head shakes and rolls, weaker as it tires
                f32 fight = 0.3f + 0.7f * (this->stamina / this->staminaMax);

                Math_SmoothStepToS(&this->actor.shape.rot.y,
                                   this->runYaw + (s16)(Math_SinS(this->timer * 0x1400) * 0x700 * fight), 3, 0x1000,
                                   0x100);
                this->actor.shape.rot.z = Math_SinS(this->timer * 0x1C00) * 0xA00 * fight;
                this->swimPhaseStep = 0.35f + (this->stamina / this->staminaMax) * 0.3f;
                this->swimMag = 1800.0f;
            }

            // splashy fight at the surface
            if (((this->timer % 12) == 0) && !this->jumping) {
                Vec3f ripple = this->actor.world.pos;

                ripple.y = this->waterY;
                EffectSsGRipple_Spawn(play, &ripple, 40, 200, 0);
            }
            break;
        }

        case FISH_STATE_CAUGHT: {
            Player* player = GET_PLAYER(play);
            f32 height = LINK_IS_CHILD ? 32.0f : 50.0f;

            // held up in front of Link, flapping
            this->actor.world.pos.x = player->actor.world.pos.x + Math_SinS(player->actor.shape.rot.y) * 16.0f;
            this->actor.world.pos.y = player->actor.world.pos.y + height;
            this->actor.world.pos.z = player->actor.world.pos.z + Math_CosS(player->actor.shape.rot.y) * 16.0f;
            this->actor.shape.rot.y = player->actor.shape.rot.y + 0x4000;
            this->actor.shape.rot.x = 0;
            this->actor.shape.rot.z = Math_SinS(this->timer * 0x1800) * 0x800;
            this->swimPhaseStep = 0.6f;
            this->swimMag = 2500.0f;
            break;
        }

        case FISH_STATE_FLEE: {
            Vec3f away;

            // pick a way out once (away from the bobber, toward open water), and again only if the shore is
            // in the way
            if (this->stateTimer == 49) {
                this->runYaw = EnFishmod_OpenWaterYaw(this, play,
                                                      Math_Vec3f_Yaw(&rod->bobberPos, &this->actor.world.pos));
            }
            away.x = this->actor.world.pos.x + Math_SinS(this->runYaw) * 60.0f;
            away.y = this->actor.world.pos.y - 5.0f;
            away.z = this->actor.world.pos.z + Math_CosS(this->runYaw) * 60.0f;
            if (!EnFishmod_SwimToward(this, play, &away, 2.0f + b->speed * 0.02f, 0x800)) {
                this->runYaw = EnFishmod_OpenWaterYaw(this, play, this->runYaw);
            }
            this->swimPhaseStep = 0.6f;
            this->swimMag = 1800.0f;
            this->jawRot = 0;
            this->actor.shape.rot.z = 0;
            if (this->stateTimer == 0) {
                EnFishmod_FishSetState(this, FISH_STATE_WANDER);
            }
            break;
        }

        case FISH_STATE_PHOTO:
            this->actor.world.pos = this->actor.home.pos;
            this->actor.shape.rot.y = gFishmodPhoto.yaw;
            this->actor.shape.rot.x = gFishmodPhoto.pitch;
            // a gentle, nearly straight pose for portraits
            this->swimMag = 250.0f;
            this->swimPhaseStep = 0.08f;
            break;
    }
}

static void EnFishmod_UpdateSwim(EnFishmod* this) {
    s32 i;

    this->swimPhase += this->swimPhaseStep;
    if (this->swimPhase > 2.0f * M_PI) {
        this->swimPhase -= 2.0f * M_PI;
    }

    // traveling wave down the body, stronger toward the tail
    for (i = 0; i < 4; i++) {
        this->bodyYaw[i] = cosf(this->swimPhase - (i * 1.1f)) * this->swimMag * (0.6f + (i * 0.45f));
    }
    this->finYaw = (Math_SinS(this->timer * 0x900) * 1800.0f) + 1800.0f;
}

#if FISHMOD_DEBUG
static void EnFishmod_UpdateImpl(Actor* thisx, PlayState* play);
static void EnFishmod_DrawImpl(Actor* thisx, PlayState* play);

void EnFishmod_Update(Actor* thisx, PlayState* play) {
    OSTime start = osGetTime();

    EnFishmod_UpdateImpl(thisx, play);
    gFishmodCpuTime[(((EnFishmod*)thisx)->type == FISHMOD_TYPE_ROD) ? FISHMOD_CPU_ROD_UPDATE
                                                                    : FISHMOD_CPU_FISH_UPDATE] +=
        osGetTime() - start;
}

void EnFishmod_Draw(Actor* thisx, PlayState* play) {
    OSTime start = osGetTime();

    EnFishmod_DrawImpl(thisx, play);
    gFishmodCpuTime[(((EnFishmod*)thisx)->type == FISHMOD_TYPE_ROD) ? FISHMOD_CPU_ROD_DRAW : FISHMOD_CPU_FISH_DRAW] +=
        osGetTime() - start;
}

#define EnFishmod_Update EnFishmod_UpdateImpl
#define EnFishmod_Draw EnFishmod_DrawImpl
#endif

void EnFishmod_Update(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;

    this->timer++;

    if (this->type == FISHMOD_TYPE_ROD) {
        EnFishmod_RodUpdate(this, play);
        gFishmodRodState.splashed = false;
        return;
    }

#if FISHMOD_OPT >= 2
    // far from Link and the bobber, a wandering fish thinks every other frame and glides on in between
    if ((this->fishState == FISH_STATE_WANDER) && (this->timer & 1) && !gFishmodPhoto.active &&
        (Math_Vec3f_DistXZ(&this->actor.world.pos, &GET_PLAYER(play)->actor.world.pos) > 500.0f) &&
        (!EnFishmod_RodBobberInWater() ||
         (Math_Vec3f_DistXZ(&this->actor.world.pos, &gFishmodRodState.bobberPos) > 300.0f))) {
        this->actor.world.pos.x += Math_SinS(this->actor.shape.rot.y) * this->speed;
        this->actor.world.pos.z += Math_CosS(this->actor.shape.rot.y) * this->speed;
        if (this->stateTimer > 0) {
            this->stateTimer--;
        }
        if (this->cooldown > 0) {
            this->cooldown--;
        }
        EnFishmod_UpdateSwim(this);
        return;
    }
#endif
    EnFishmod_FishAI(this, play);
    EnFishmod_UpdateSwim(this);

#if FISHMOD_DEBUG
    {
        // nearest fish to the camera focus, for testing
        static const char* sStateNames[] = { "wander", "approach", "inspect", "nibble", "bite",
                                             "hooked", "caught",   "flee",    "photo" };
        static u32 sFrame;
        static f32 sBestDist;
        static s32 sCount;
        Player* player = GET_PLAYER(play);
        f32 dist = EnFishmod_RodBobberInWater()
                       ? Math_Vec3f_DistXZ(&this->actor.world.pos, &gFishmodRodState.bobberPos)
                       : Math_Vec3f_DistXZ(&this->actor.world.pos, &player->actor.world.pos);
        static s32 sClipFrames;
        static f32 sClipMax;
        static s32 sClipState;
        static s32 sClipSize;

        // how far any fish's body pokes out of the water or into the floor
        if ((this->fishState != FISH_STATE_CAUGHT) && (this->fishState != FISH_STATE_PHOTO) && !this->jumping &&
            !(this->fishState == FISH_STATE_HOOKED && (this->waterY - this->floorY) < this->halfHeight * 2.0f)) {
            f32 surface;
            f32 floor;
            f32 out = 0.0f;

            if (EnFishmod_WaterAt(play, this->actor.world.pos.x, this->actor.world.pos.z, &surface, &floor)) {
                out = CLAMP_MIN(this->actor.world.pos.y + this->halfHeight - surface,
                                floor - (this->actor.world.pos.y - this->halfHeight));
            }
            if (out > 1.0f) {
                sClipFrames++;
                if (out > sClipMax) {
                    sClipMax = out;
                    sClipState = this->fishState;
                    sClipSize = (s32)this->lengthInches;
                }
            }
        }
        // biggest one-frame heading change while at the bobber (the old "flashing")
        {
            static s32 sTurnMax;
            s32 turn = ABS((s16)(this->actor.shape.rot.y - this->actor.world.rot.y));

            if (((this->fishState == FISH_STATE_INSPECT) || (this->fishState == FISH_STATE_NIBBLE) ||
                 (this->fishState == FISH_STATE_BITE)) &&
                (turn > sTurnMax)) {
                sTurnMax = turn;
            }
            this->actor.world.rot.y = this->actor.shape.rot.y;
            Fishmod_DebugPrintf(8, "fish clip %d max %d turn %X", sClipFrames, (s32)sClipMax, sTurnMax);
        }

        if (sFrame != play->gameplayFrames) {
            sFrame = play->gameplayFrames;
            sBestDist = 99999.0f;
            sCount = 0;
        }
        sCount++;
        if (dist < sBestDist) {
            sBestDist = dist;
            gFishmodDebugFocus = this->actor.world.pos;
            gFishmodDebugFocusYaw = this->actor.shape.rot.y;
            Fishmod_DebugPrintf(5, "fish%d %s %s", sCount, this->species->name, sStateNames[this->fishState]);
            Fishmod_DebugPrintf(6, " d%d blk%d det%d cd%d", (s32)dist, this->blocked, this->detourTimer, this->cooldown);
        }
    }
#endif
}

s32 EnFishmod_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    EnFishmod* this = (EnFishmod*)thisx;
    s32 li = limbIndex - 1;
    u8* partMap = EnFishmod_PartMap(this->species);
    u8 part;
    Vec3f* scale;
    Vec3f parentScale;

    if (li >= this->limbCount) {
        return false;
    }

    if ((this->species->base == FISHMOD_BASE_SMALL) && (*dList != NULL)) {
        s32 k;

        for (k = 0; k < 3; k++) {
            if (*dList == sSmallDLSources[k]) {
                *dList = sSmallDLs[k];
                break;
            }
        }
    }

    // swimming
    if (this->species->base == FISHMOD_BASE_SMALL) {
        if (limbIndex == 4) {
            rot->y += this->bodyYaw[1];
        } else if (limbIndex == 6) {
            rot->y += this->bodyYaw[3];
        }
    } else if (this->species->base == FISHMOD_BASE_LOACH) {
        if ((limbIndex >= 3) && (limbIndex <= 5)) {
            rot->y += this->bodyYaw[limbIndex - 2];
        } else if ((limbIndex == 9) || (limbIndex == 10)) {
            rot->y += (limbIndex == 9) ? this->finYaw : -this->finYaw;
        }
    } else {
        if ((limbIndex == 2) || (limbIndex == 3)) {
            rot->y += this->bodyYaw[limbIndex - 1];
        } else if (limbIndex == 4) {
            rot->y += this->bodyYaw[3];
        } else if (limbIndex == 0xD) {
            rot->z -= this->jawRot;
        } else if (limbIndex == 0xE) {
            rot->y -= this->finYaw;
        } else if (limbIndex == 0xF) {
            rot->y += this->finYaw;
        }
    }

    // color
    part = partMap[li];
#if FISHMOD_OPT >= 1
    if ((part != FISHMOD_PART_NONE) && (part != this->lastTintPart)) {
        const u8* tint = this->species->partTint[part];

        this->lastTintPart = part;
#else
    if (part != FISHMOD_PART_NONE) {
        const u8* tint = this->species->partTint[part];
#endif

        OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, tint[0], tint[1], tint[2], 255);
        CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
    }

    // Per-part scale. Scaling a limb would also scale its children, so each limb undoes its parent's scale,
    // moves its joint by the parent's scale so it stays attached, then applies its own scale.
    scale = &this->limbScale[li];
    if (this->limbParent[li] != 0xFF) {
        parentScale = this->limbScale[this->limbParent[li]];
    } else {
        parentScale.x = parentScale.y = parentScale.z = 1.0f;
    }

#if FISHMOD_OPT >= 1
    switch (this->limbMode[li]) {
        case LIMB_MODE_OWN_SCALE:
            Matrix_TranslateRotateZYX(pos, rot);
            Matrix_Scale(scale->x, scale->y, scale->z, MTXMODE_APPLY);
            pos->x = pos->y = pos->z = 0.0f;
            rot->x = rot->y = rot->z = 0;
            return false;
        case LIMB_MODE_UNDO_PARENT:
            Matrix_Scale(1.0f / parentScale.x, 1.0f / parentScale.y, 1.0f / parentScale.z, MTXMODE_APPLY);
            pos->x *= parentScale.x;
            pos->y *= parentScale.y;
            pos->z *= parentScale.z;
            return false;
        case LIMB_MODE_PLAIN:
            return false;
        default:
            break;
    }
#endif
    if (this->limbMode[li] != LIMB_MODE_PLAIN) {
        Matrix_Scale(1.0f / parentScale.x, 1.0f / parentScale.y, 1.0f / parentScale.z, MTXMODE_APPLY);
        pos->x *= parentScale.x;
        pos->y *= parentScale.y;
        pos->z *= parentScale.z;
        Matrix_TranslateRotateZYX(pos, rot);
        Matrix_Scale(scale->x, scale->y, scale->z, MTXMODE_APPLY);
        pos->x = pos->y = pos->z = 0.0f;
        rot->x = rot->y = rot->z = 0;
    }

    return false;
}

// Whiskers in fish space, as fractions of the fish's length: { forward, up, side } for root, middle, tip.
// Mirrored on the side axis for the other whisker of each pair.
static Vec3f sWhiskerPoints[][3] = {
    { { 0.00f, -0.01f, 0.03f }, { 0.04f, -0.05f, 0.12f }, { -0.02f, -0.13f, 0.20f } }, // long upper pair
    { { 0.00f, -0.03f, 0.015f }, { 0.02f, -0.08f, 0.04f }, { -0.01f, -0.14f, 0.06f } }, // short chin pair
};

static Vec3f sWhiskerWorld[4][3];

/**
 * Places the whiskers at the snout using the fish's facing, so they hang forward and down like barbels.
 */
static void EnFishmod_UpdateWhiskers(EnFishmod* this) {
    f32 dx = this->boundsMax.x - this->boundsMin.x;
    f32 dz = this->boundsMax.z - this->boundsMin.z;
    f32 length = sqrtf(SQ(dx) + SQ(dz)) * 1.9f;
    f32 fwdX = Math_SinS(this->actor.shape.rot.y);
    f32 fwdZ = Math_CosS(this->actor.shape.rot.y);
    s32 i;
    s32 k;
    s32 side;

    for (i = 0; i < 2; i++) {
        for (k = 0; k < 3; k++) {
            Vec3f* p = &sWhiskerPoints[i][k];
            f32 sway = Math_SinS(this->timer * 0x500 + k * 0x2400) * 0.01f * k;

            for (side = 0; side < 2; side++) {
                f32 sideAmt = (side == 0) ? p->z : -p->z;
                Vec3f* out = &sWhiskerWorld[i * 2 + side][k];

                // snout: the head limb origin pushed forward a little
                out->x = this->headPos.x + (fwdX * (p->x + 0.03f) + fwdZ * (sideAmt + sway)) * length;
                out->y = this->headPos.y + p->y * length;
                out->z = this->headPos.z + (fwdZ * (p->x + 0.03f) - fwdX * (sideAmt + sway)) * length;
            }
        }
    }
}

void EnFishmod_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx) {
    EnFishmod* this = (EnFishmod*)thisx;
    static Vec3f sZero = { 0.0f, 0.0f, 0.0f };
    s32 i;
    s32 k;
    Vec3f limbPos;

    Matrix_MultVec3f(&sZero, &limbPos);
    if (limbIndex == 1) {
        this->boundsMin = this->boundsMax = limbPos;
    } else {
        this->boundsMin.x = CLAMP_MAX(this->boundsMin.x, limbPos.x);
        this->boundsMin.y = CLAMP_MAX(this->boundsMin.y, limbPos.y);
        this->boundsMin.z = CLAMP_MAX(this->boundsMin.z, limbPos.z);
        this->boundsMax.x = CLAMP_MIN(this->boundsMax.x, limbPos.x);
        this->boundsMax.y = CLAMP_MIN(this->boundsMax.y, limbPos.y);
        this->boundsMax.z = CLAMP_MIN(this->boundsMax.z, limbPos.z);
    }

    if (limbIndex != 1) {
        return;
    }

    this->headPos = limbPos;

}

static void EnFishmod_DrawWhiskers(EnFishmod* this, PlayState* play) {
    s32 i;
    s32 k;
    const u8* tint = this->species->partTint[FISHMOD_PART_HEAD];

    OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);

    POLY_OPA_DISP = Gfx_SetupDL(POLY_OPA_DISP, SETUPDL_25);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, tint[0] / 2, tint[1] / 2, tint[2] / 2, 255);

    for (i = 0; i < ARRAY_COUNT(sWhiskerWorld); i++) {
        for (k = 0; k < 2; k++) {
            Vec3f* a = &sWhiskerWorld[i][k];
            Vec3f* b = &sWhiskerWorld[i][k + 1];
            f32 dx = b->x - a->x;
            f32 dy = b->y - a->y;
            f32 dz = b->z - a->z;
            f32 distXZ = sqrtf(SQ(dx) + SQ(dz));
            f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));

            Matrix_Translate(a->x, a->y, a->z, MTXMODE_NEW);
            Matrix_RotateY(Math_FAtan2F(dx, dz), MTXMODE_APPLY);
            Matrix_RotateX(-Math_FAtan2F(dy, distXZ), MTXMODE_APPLY);
            Matrix_Scale(0.005f * this->actor.scale.z * 300.0f * (1.0f - k * 0.45f), 1.0f, dist * 0.001f,
                         MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
            gSPDisplayList(POLY_OPA_DISP++, gFishingLineModelDL);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
}

/**
 * Photo mode backdrop: a vertical gradient over the whole screen, drawn before the fish.
 * Rooms and the skybox are not drawn in photo mode, so nothing else is in the frame.
 */
static void EnFishmod_DrawPhotoBackdrop(PlayState* play) {
    s32 band;

    OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCycleType(POLY_OPA_DISP++, G_CYC_FILL);
    gDPSetRenderMode(POLY_OPA_DISP++, G_RM_NOOP, G_RM_NOOP2);

    for (band = 0; band < 24; band++) {
        // pale water blue at the top to a deeper blue-green at the bottom
        s32 r = 200 - band * 4;
        s32 g = 225 - band * 3;
        s32 b = 230 - band * 2;
        u16 color = GPACK_RGBA5551(r, g, b, 1);

        gDPSetFillColor(POLY_OPA_DISP++, (color << 16) | color);
        gDPFillRectangle(POLY_OPA_DISP++, 0, band * 10, SCREEN_WIDTH - 1, band * 10 + 9);
    }

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCycleType(POLY_OPA_DISP++, G_CYC_1CYCLE);

    // capture marker: tells the capture script which photo is on screen
    gDPSetCycleType(OVERLAY_DISP++, G_CYC_FILL);
    gDPSetRenderMode(OVERLAY_DISP++, G_RM_NOOP, G_RM_NOOP2);
    {
        // marker n -> red = 16 * n, so the capture script can read it back
        u16 markerColor = GPACK_RGBA5551(gFishmodPhoto.marker * 16, 0, 255, 1);

        gDPSetFillColor(OVERLAY_DISP++, (markerColor << 16) | markerColor);
    }
    gDPFillRectangle(OVERLAY_DISP++, 4, 4, 11, 11);
    gDPPipeSync(OVERLAY_DISP++);
    gDPSetCycleType(OVERLAY_DISP++, G_CYC_1CYCLE);

    CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
}

#if FISHMOD_OPT >= 2
/**
 * Draws the skeleton. A distant fish that's just swimming around redraws the pose it had last frame every other
 * frame: the limb matrices and the commands that drew them are kept, copied into this frame and pointed at the
 * copied matrices, which skips all the limb math. It moves slowly and is small on screen, so the half-rate
 * animation doesn't show.
 */
static void EnFishmod_DrawPose(EnFishmod* this, PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx* start;
    s32 reuse = false;

    if (!gFishmodPhoto.active && (this->poseFrame + 1 == play->gameplayFrames) && (this->poseGfxCount != 0) &&
        ((this->fishState == FISH_STATE_WANDER) || (this->fishState == FISH_STATE_FLEE)) &&
        (Math_Vec3f_DistXYZ(&this->actor.world.pos, &play->view.eye) > 350.0f)) {
        reuse = true;
    }

    OPEN_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);

    if (reuse) {
        u32 size = this->poseMtxCount * sizeof(Mtx);
        Mtx* mtx = GRAPH_ALLOC(gfxCtx, size);
        u32 oldBase = this->poseGfx[0].words.w1; // the segment 0x0D command points at the old matrices
        s32 i;

        bcopy(this->poseMtx, mtx, size);
        for (i = 0; i < this->poseGfxCount; i++) {
            Gfx g = this->poseGfx[i];
            u8 op = g.words.w0 >> 24;

            if (((op == G_MTX) || ((op == G_MOVEWORD) && (((g.words.w0 >> 16) & 0xFF) == G_MW_SEGMENT))) &&
                (g.words.w1 >= oldBase) && (g.words.w1 < oldBase + size)) {
                g.words.w1 = g.words.w1 - oldBase + (u32)mtx;
            }
            *(POLY_OPA_DISP++) = g;
        }
        // draw it properly next frame
        this->poseFrame = 0;
    } else {
        start = POLY_OPA_DISP;
        SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable,
                              this->skelAnime.dListCount, EnFishmod_OverrideLimbDraw, EnFishmod_PostLimbDraw, this);
        this->poseGfxCount = 0;
        if (((POLY_OPA_DISP - start) <= EN_FISHMOD_POSE_GFX_MAX) &&
            (this->skelAnime.dListCount <= EN_FISHMOD_MAX_LIMBS + 1) && ((start[0].words.w0 >> 24) == G_MOVEWORD)) {
            this->poseGfxCount = POLY_OPA_DISP - start;
            this->poseMtxCount = this->skelAnime.dListCount;
            bcopy(start, this->poseGfx, this->poseGfxCount * sizeof(Gfx));
            bcopy((void*)start[0].words.w1, this->poseMtx, this->poseMtxCount * sizeof(Mtx));
            this->poseFrame = play->gameplayFrames;
        }
    }

    CLOSE_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
}
#endif

void EnFishmod_Draw(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;
    const u8* shape;

    if (this->type == FISHMOD_TYPE_ROD) {
        EnFishmod_RodDraw(this, play);
        return;
    }

    if (gFishmodPhoto.active && (gFishmodPhoto.backdropFrame != play->gameplayFrames)) {
        gFishmodPhoto.backdropFrame = play->gameplayFrames;
        EnFishmod_DrawPhotoBackdrop(play);
    }

    Gfx_SetupDL_25Opa(play->state.gfxCtx);

    {
        s32 texSet = EnFishmod_TexSet(this->species);
        s32 speciesIndex = this->species - gFishmodSpecies;
        s32 role;
        IF_F3DEX3_DONT_SKIP_TEX_INIT();

        OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
        // every species draws the same display lists with its own textures behind the segments
        IF_F3DEX3_DONT_SKIP_TEX_HERE(POLY_OPA_DISP++, speciesIndex);
#if FISHMOD_OPT >= 2
        // palette textures: every role has one, and the species' palette goes into the upper half of TMEM
        if (sSpeciesPalettes[speciesIndex] != NULL) {
            gDPLoadTLUT_pal256(POLY_OPA_DISP++, sSpeciesPalettes[speciesIndex]);
        }
        for (role = 0; role < FISHMOD_TEX_ROLE_MAX; role++) {
            void* tex = sSpeciesTexturesCI[speciesIndex][role];

            if (tex != NULL) {
                gSPSegment(POLY_OPA_DISP++, 0x08 + role, tex);
            }
        }
        (void)texSet;
#else
        for (role = 0; role < FISHMOD_TEX_ROLE_MAX; role++) {
            void* tex = sSpeciesTextures[speciesIndex][role];

            if (tex == NULL) {
                tex = sTextureRoles[texSet][role];
                if (tex == NULL) {
                    continue;
                }
                tex = SEGMENTED_TO_VIRTUAL(tex);
            }
            gSPSegment(POLY_OPA_DISP++, 0x08 + role, tex);
        }
#endif
        CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
    }

    shape = this->species->shape;

    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    Matrix_RotateY(BINANG_TO_RAD_ALT(this->actor.shape.rot.y), MTXMODE_APPLY);
    Matrix_RotateX(BINANG_TO_RAD_ALT(this->actor.shape.rot.x), MTXMODE_APPLY);
    Matrix_RotateZ(BINANG_TO_RAD_ALT(this->actor.shape.rot.z), MTXMODE_APPLY);
    Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);

    if (this->species->base == FISHMOD_BASE_SMALL) {
        // the bottle fish's animation already turns its root, so no quarter turn here
        Matrix_RotateY(BINANG_TO_RAD(this->bodyYaw[0]) * 0.5f, MTXMODE_APPLY);
        Matrix_Scale(shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f, MTXMODE_APPLY);
        Matrix_Translate(-650.0f, 0.0f, 0.0f, MTXMODE_APPLY);
    } else if (this->species->base == FISHMOD_BASE_LOACH) {
        Matrix_RotateY(BINANG_TO_RAD(this->bodyYaw[0]) * 0.5f - (M_PI / 2), MTXMODE_APPLY);
        Matrix_Scale(shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f, MTXMODE_APPLY);
        // center the model on the actor: the loach spans about x 5500..18600
        Matrix_Translate(-8000.0f, 500.0f, 0.0f, MTXMODE_APPLY);
    } else {
        Matrix_RotateY(BINANG_TO_RAD(this->bodyYaw[0]) * 0.5f - (M_PI / 2), MTXMODE_APPLY);
        Matrix_Scale(shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f, MTXMODE_APPLY);
        // the bass spans about x 1800..6100
        Matrix_Translate(-2300.0f, -300.0f, 0.0f, MTXMODE_APPLY);
    }

    this->lastTintPart = 0xFE;
#if FISHMOD_OPT >= 2
    EnFishmod_DrawPose(this, play);
#else
    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount,
                          EnFishmod_OverrideLimbDraw, EnFishmod_PostLimbDraw, this);
#endif

    if (this->species->lookFlags & FISHMOD_LOOK_WHISKERS) {
        EnFishmod_UpdateWhiskers(this);
        EnFishmod_DrawWhiskers(this, play);
    }

    if (gFishmodPhoto.active) {
        f32 dx = this->boundsMax.x - this->boundsMin.x;
        f32 dy = this->boundsMax.y - this->boundsMin.y;
        f32 dz = this->boundsMax.z - this->boundsMin.z;

        gFishmodPhoto.center.x = (this->boundsMin.x + this->boundsMax.x) * 0.5f;
        gFishmodPhoto.center.y = (this->boundsMin.y + this->boundsMax.y) * 0.5f;
        gFishmodPhoto.center.z = (this->boundsMin.z + this->boundsMax.z) * 0.5f;
        gFishmodPhoto.extent = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    }
}
