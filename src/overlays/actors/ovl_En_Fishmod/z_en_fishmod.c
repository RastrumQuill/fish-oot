/*
 * File: z_en_fishmod.c
 * Overlay: ovl_En_Fishmod
 * Description: fishmod overworld fishing. Type FISHMOD_TYPE_FISH is one fish of a species from
 * fishmod/data/species.json, drawn by reshaping and recoloring the Fishing Pond bass and loach models.
 * Type FISHMOD_TYPE_ROD is the per-scene fishing controller (rod, line, bobber, camera, spawning).
 */

#include "z_en_fishmod.h"
#include "sys_math.h"
#include "assets/objects/gameplay_field_keep/gameplay_field_keep.h"

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

u8* EnFishmod_PartMap(const FishmodSpecies* species) {
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
#if FISHMOD_OPT >= 5
            } else if ((texSet == TEXSET_BASS) && (opcode == G_GEOMETRYMODE) && (gfx->words.w1 == G_CULL_BACK) &&
                       ((gfx->words.w0 & 0xFFFFFF) == 0xFFFFFF)) {
                // culling is set per fish instead (EnFishmod_Draw, EnFishmod_RigDraw): off while the mouth is
                // open, when you see the inside of the head and body through it
                gfx->words.w0 = G_NOOP << 24;
                gfx->words.w1 = 0;
#endif
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

/** The overlay's patched copy of one of the small fish's display lists (the original if it isn't one of them). */
Gfx* EnFishmod_SmallDL(Gfx* src) {
    s32 k;

    for (k = 0; k < 3; k++) {
        if (src == sSmallDLSources[k]) {
            return sSmallDLs[k];
        }
    }
    return SEGMENTED_TO_VIRTUAL(src);
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

#if FISHMOD_OPT >= 4
    if (this->type == FISHMOD_TYPE_MANAGER) {
        EnFishmod_ManagerInit(this, play);
        return;
    }
#endif
    if (this->type == FISHMOD_TYPE_ROD) {
        this->actionFunc = EnFishmod_RodIdle;
        EnFishmod_RodInit(this, play);
        return;
    }
    if (this->type == FISHMOD_TYPE_COMB) {
        // a chunk of broken beehive: pops out, falls, and lies there for a while to be scooped up
        Actor_SetScale(&this->actor, 0.05f); // half a hive: big enough to spot beside Link
        this->actor.gravity = -1.0f;
        this->actor.velocity.y = 3.0f;
        this->actor.speed = 1.5f;
        this->actor.world.rot.y = this->actor.shape.rot.y;
        this->stateTimer = 30 * 20;
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
        EnFishmod_SetLength(this, (gFishmodStudio.active && (gFishmodStudio.spawnLength > 0.0f))
                                      ? gFishmodStudio.spawnLength
                                      : this->species->lengthTypical);
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
        if (gFishmodSpawnLength > 0.0f) {
            // poured back from a bottle: its own size, and off it goes
            EnFishmod_SetLength(this, gFishmodSpawnLength);
            this->fishState = FISH_STATE_FLEE;
            this->stateTimer = 50;
            this->cooldown = 600;
        }
        // don't draw fish that are off screen or far away (they still swim)
        this->actor.flags &= ~ACTOR_FLAG_DRAW_CULLING_DISABLED;
        this->actor.cullingVolumeDistance = FISHMOD_CULL_DIST;
        this->actor.cullingVolumeScale = this->halfLength * 2.0f + 20.0f;
        this->actor.cullingVolumeDownward = this->halfLength * 2.0f + 20.0f;
    }
#if FISHMOD_OPT >= 4
    this->rosterIndex = FISHMOD_NO_ROSTER;
    {
#if FISHMOD_OPT >= 5
        // the manager passes the record's index in the spawn roll
        s32 index = thisx->world.rot.z - 1;

        thisx->world.rot.z = thisx->shape.rot.z = 0;
#else
        s32 index = FISHMOD_GET_ROSTER(thisx->params);
#endif

        if (!gFishmodPhoto.active && (index >= 0) && (EnFishmod_RosterLength(index) > 0.0f)) {
            // one of the scene's roster fish: its own size, every time it appears
            this->rosterIndex = index;
            EnFishmod_SetLength(this, EnFishmod_RosterLength(this->rosterIndex));
        }
    }
    // the fish manager draws every fish (if this species' rig couldn't be made, it draws itself the old way)
    this->rigReady = EnFishmod_RigPrepare(this);
    if (this->rigReady) {
        this->actor.draw = NULL;
    }
    EnFishmod_ManagerEnsure(play);
#endif
    this->staminaMax = 60.0f + this->species->behavior.stamina * 3.0f + this->lengthInches * 4.0f;
    this->stamina = this->staminaMax;
    this->swimPhaseStep = 0.25f;
    this->swimMag = 1500.0f;
    this->swimPhase = Rand_ZeroFloat(6.28f);
    this->depthPref = (u8)Rand_ZeroFloat(255.9f);
#if FISHMOD_OPT >= 5
    this->waterCell = -2;
#endif
    EnFishmod_WaterGridAcquire(play);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    this->actionFunc = EnFishmod_FishHover;
}

void EnFishmod_Destroy(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;

    if (this->type == FISHMOD_TYPE_COMB) {
        return;
    }
#if FISHMOD_OPT >= 4
    if (this->type == FISHMOD_TYPE_MANAGER) {
        EnFishmod_ManagerDestroy(this, play);
        return;
    }
    if (this->type == FISHMOD_TYPE_FISH) {
        EnFishmod_RosterForget(this);
    }
#endif
    if (this->type == FISHMOD_TYPE_ROD) {
        EnFishmod_RodDestroy(this, play);
        if (gFishmodRod == thisx) {
            gFishmodRod = NULL;
        }
    } else {
        EnFishmod_RodRelease(this);
        SkelAnime_Free(&this->skelAnime, play);
        EnFishmod_WaterGridRelease(play);
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
#if FISHMOD_OPT >= 5
    this->waterCell = -2; // not the steering's cell answer any more
#endif
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

#if FISHMOD_OPT >= 5
/** The time of day on this fish's own clock (carp differ by individual: FishmodSpecies.chronotype). */
static s32 EnFishmod_TimeOfDay(EnFishmod* this) {
    s32 shift = 0;
    s32 time;

    if (this->species->chronotype != 0) {
        s32 spread = this->species->chronotype * 2 + 1;

        shift = (this->depthPref % spread) - this->species->chronotype;
    }
    time = (u16)(gSaveContext.save.dayTime + shift * CLOCK_TIME(1, 0));
    if ((time >= CLOCK_TIME(4, 0)) && (time < CLOCK_TIME(8, 0))) {
        return FISHMOD_TIME_DAWN;
    } else if ((time >= CLOCK_TIME(8, 0)) && (time < CLOCK_TIME(17, 0))) {
        return FISHMOD_TIME_DAY;
    } else if ((time >= CLOCK_TIME(17, 0)) && (time < CLOCK_TIME(20, 0))) {
        return FISHMOD_TIME_DUSK;
    }
    return FISHMOD_TIME_NIGHT;
}

/** How active it is now, 0-3 (its species' rhythm on its own clock). */
static s32 EnFishmod_Activity(EnFishmod* this) {
    return FISHMOD_ACTIVITY(this->species, EnFishmod_TimeOfDay(this));
}

/**
 * Where in the water column it keeps (AI v2): its spot in the species' band, higher when it's feeding (active, in
 * low light: deep fish come up at dawn and dusk, night feeders at night) and deeper in bright light for fish that
 * shy from it (research: fishmod/docs/research/biomes-time-behaviour.md).
 */
static f32 EnFishmod_DepthY(EnFishmod* this, PlayState* play, f32 surface, f32 floor) {
    const FishmodSpecies* sp = this->species;
    f32 clearance = EnFishmod_BodyClearance(this);
    f32 light = EnFishmod_Shared(play)->light;
    f32 activity = EnFishmod_Activity(this) * (1.0f / 3.0f);
    f32 pref = this->depthPref * (1.0f / 255.0f);
    f32 f = (sp->depthTop + (sp->depthBottom - sp->depthTop) * pref) * (1.0f / 255.0f);
    f32 room = (surface - clearance) - (floor + clearance);

    if (activity >= 0.6f) {
        f -= sp->rise * (1.0f / 255.0f) * activity * (1.0f - light);
    }
    if (light > 0.5f) {
        f += sp->lightShy * (1.0f / 255.0f) * (light - 0.5f) * 0.6f;
    }
    f = CLAMP(f, 0.0f, 1.0f);
    if (room <= 0.0f) {
        return (surface + floor) * 0.5f;
    }
    return surface - clearance - f * room;
}
#else
static f32 EnFishmod_DepthY(EnFishmod* this, PlayState* play, f32 surface, f32 floor) {
    f32 depth = surface - floor;
    // each fish has its own spot in its species' band, so deep water fills in layers instead of one plane each
    f32 pref = this->depthPref * (1.0f / 255.0f);
    f32 band = CLAMP_MAX(depth * 0.25f, 50.0f);

    switch (this->species->depth) {
        case FISHMOD_DEPTH_SURFACE:
            return surface - CLAMP_MAX(6.0f + pref * band, depth * 0.5f);
        case FISHMOD_DEPTH_BOTTOM:
            return floor + CLAMP_MAX(5.0f + pref * band, depth * 0.4f);
        default:
            return floor + depth * (0.3f + pref * 0.4f);
    }
}
#endif

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
#if FISHMOD_OPT >= 5
    {
        static f32 sActivityAppeal[] = { 0.4f, 0.7f, 1.1f, 1.4f };

        appeal *= sActivityAppeal[EnFishmod_Activity(this)];
    }
    (void)tod;
#else
    appeal *= (this->species->activeTimes & (1 << tod)) ? 1.3f : 0.6f;
#endif
    if (gFishmodRodState.twitchTimer > 0) {
        appeal *= 1.5f;
    }
    return appeal;
}

/** True during this species' active times of day. */
static s32 EnFishmod_ActiveNow(EnFishmod* this) {
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
    return (this->species->activeTimes & (1 << tod)) != 0;
}

static void EnFishmod_PickWanderTarget(EnFishmod* this, PlayState* play) {
    Vec3f point;
    f32 surface;
    f32 floor;

    if (EnFishmod_FindWaterPoint(play, &this->actor.home.pos, 75.0f,
                                 CLAMP_MIN(12.0f, this->halfHeight * 2.0f + this->halfLength * 0.5f), &point,
                                 &surface, &floor)) {
        point.y = EnFishmod_DepthY(this, play, surface, floor);
        if ((Rand_ZeroOne() < 0.25f) && (point.y - floor < 80.0f)) {
            // down to pick at the bottom, if it's near
            point.y = floor + this->halfHeight + 1.0f;
        }
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
#if FISHMOD_OPT >= 5
    if ((state != FISH_STATE_HOOKED) && (state != FISH_STATE_NIBBLE) && (state != FISH_STATE_BITE)) {
        this->jawRot = 0; // shuts its mouth (an escaped fish kept the bite's or the fight's gape)
    }
#endif

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
            this->startle = (Rand_ZeroOne() < 0.5f) ? 8 : -8;
            this->restTimer = 0;
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

#if FISHMOD_OPT >= 5
    {
        // the open water map knows which way is away from the banks: the side of that the fish already favours
        s16 away;

        if (EnFishmod_OpenWaterDir(play, this->actor.world.pos.x, this->actor.world.pos.z, &away)) {
            s16 diff = preferred - away;

            // within 45 degrees of straight away from the bank, on the side it was already turning to
            return away + CLAMP(diff, -0x2000, 0x2000);
        }
    }
#endif

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
#if FISHMOD_OPT >= 5
/**
 * Steers toward target at the given speed, staying inside the water (AI v2): a heading with pitch, so a fish swims
 * up or down to its target instead of easing its height. The water under it is looked up only when it moves into
 * another grid cell, and the way ahead is checked every few frames at one point a little in front of its nose; a
 * bank there turns it toward open water (the open water map) and it slides along. Returns false if the shore
 * blocked it.
 */
static s32 EnFishmod_SwimTowardImpl(EnFishmod* this, PlayState* play, Vec3f* target, f32 speed, s16 turnStep) {
    Vec3f* pos = &this->actor.world.pos;
    f32 dx = target->x - pos->x;
    f32 dz = target->z - pos->z;
    f32 dy = target->y - pos->y;
    f32 distXZSq = SQ(dx) + SQ(dz);
    f32 clearance = EnFishmod_BodyClearance(this);
    s32 cell;
    s16 yaw;
    s16 pitch;
    f32 horiz;

    // the water here: only when it enters another cell (the floor is the cell's highest corner)
    cell = EnFishmod_WaterCell(play, pos->x, pos->z);
    if ((cell != this->waterCell) || (cell < 0)) {
        f32 surface;
        f32 floor;

        if (EnFishmod_WaterFloor(play, pos->x, pos->z, &surface, &floor, true)) {
            this->waterY = surface;
            this->floorY = floor;
        }
        this->waterCell = cell;
    }

    // somehow inside a bank: push straight out toward open water
    if ((this->waterY - this->floorY) <= 1.0f) {
        s16 out;

        if (!EnFishmod_OpenWaterDir(play, pos->x, pos->z, &out)) {
            out = Math_Vec3f_Yaw(pos, &this->actor.home.pos);
        }
        pos->x += Math_SinS(out) * 1.5f;
        pos->z += Math_CosS(out) * 1.5f;
        Math_SmoothStepToS(&this->actor.shape.rot.y, out, 2, 0x800, 0x100);
        this->waterCell = -2;
        this->blocked = true;
        return false;
    }

    if (distXZSq < SQ(2.0f + this->halfLength * 0.3f)) {
        // basically there: hold the heading and drift onto the spot (turning toward a point this close would swing
        // the fish around with every tiny move)
        f32 dist = sqrtf(distXZSq);
        f32 step = CLAMP_MAX(dist * 0.2f, speed);

        Math_ApproachF(&this->speed, 0.0f, 1.0f, 0.08f);
        if (dist > 0.01f) {
            pos->x += dx / dist * step;
            pos->z += dz / dist * step;
        }
        pos->y += CLAMP(dy * 0.08f, -0.8f, 0.8f);
        Math_SmoothStepToS(&this->actor.shape.rot.x, 0, 4, 0x200, 0x20);
    } else {
        yaw = Math_Atan2S(dz, dx);
        if (this->detourTimer > 0) {
            // working around a bank: follow the way out for a while before aiming at the target again
            this->detourTimer--;
            yaw = this->detourYaw;
            turnStep = CLAMP_MIN(turnStep, 0x400);
        }
        Math_SmoothStepToS(&this->actor.shape.rot.y, yaw, 4, turnStep, 0x40);
        // pitch toward the target's height, at most 45 degrees (rot.x is nose down positive)
        pitch = CLAMP(Math_Atan2S(sqrtf(distXZSq), dy), -0x2000, 0x2000);
        Math_SmoothStepToS(&this->actor.shape.rot.x, -pitch, 4, 0x300, 0x40);
        Math_ApproachF(&this->speed, speed, 1.0f, 0.08f);

        // the way ahead, every 4th frame (staggered by fish): one point in front of the nose
        if (((this->timer + this->depthPref) & 3) == 0) {
            f32 reach = this->halfLength + 4.0f + this->speed * 6.0f;
            f32 surface;
            f32 floor;
            f32 ax = pos->x + Math_SinS(this->actor.shape.rot.y) * reach;
            f32 az = pos->z + Math_CosS(this->actor.shape.rot.y) * reach;

            if (!EnFishmod_WaterFloor(play, ax, az, &surface, &floor, true) ||
                ((surface - floor) < clearance * 2.0f + 2.0f)) {
                // a bank ahead: turn toward open water, on the side it was already turning to, and slow down
                s16 away;

                if (EnFishmod_OpenWaterDir(play, pos->x, pos->z, &away)) {
                    s16 diff = this->actor.shape.rot.y - away;

                    away += CLAMP(diff, -0x3000, 0x3000);
                } else {
                    away = Math_Vec3f_Yaw(pos, &this->actor.home.pos);
                }
                this->detourYaw = away;
                this->detourTimer = 20;
                this->speed = CLAMP_MIN(this->speed * 0.5f, 0.4f);
                this->blocked = true;
            } else {
                this->blocked = false;
            }
        }
        horiz = this->speed * Math_CosS(this->actor.shape.rot.x);
        pos->x += Math_SinS(this->actor.shape.rot.y) * horiz;
        pos->z += Math_CosS(this->actor.shape.rot.y) * horiz;
        pos->y -= this->speed * Math_SinS(this->actor.shape.rot.x);
    }
    pos->y = EnFishmod_ClampY(this, pos->y);
    return !this->blocked;
}
#else
static s32 EnFishmod_SwimTowardImpl(EnFishmod* this, PlayState* play, Vec3f* target, f32 speed, s16 turnStep) {
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

    // Somehow inside a bank (its spot reads as dry): push straight out toward open water. No checks: from inside
    // the bank, every step looks like land.
    if (EnFishmod_WaterAt(play, this->actor.world.pos.x, this->actor.world.pos.z, &surface, &floor) &&
        ((surface - floor) <= 1.0f)) {
        s16 out = EnFishmod_OpenWaterYaw(this, play, this->actor.shape.rot.y);

        this->actor.world.pos.x += Math_SinS(out) * 1.5f;
        this->actor.world.pos.z += Math_CosS(out) * 1.5f;
        Math_SmoothStepToS(&this->actor.shape.rot.y, out, 2, 0x800, 0x100);
        this->blocked = true;
        return false;
    }

    // Stuck in a spot too tight for it (against a bank, or pushed into one): step straight along the way out
    // rather than along its heading, which still points at the bank.
    if (this->blocked && (this->detourTimer > 0)) {
        next.x = this->actor.world.pos.x + Math_SinS(this->detourYaw) * 0.6f;
        next.z = this->actor.world.pos.z + Math_CosS(this->detourYaw) * 0.6f;
    }

    if (EnFishmod_ProbeWater(this, play, &next, this->actor.shape.rot.y)) {
        next.y = EnFishmod_ClampY(this, next.y);
        this->actor.world.pos = next;
    } else if (EnFishmod_WaterAt(play, next.x, next.z, &surface, &floor) && ((surface - floor) > 1.0f) &&
               (((surface - floor) > EnFishmod_ShallowDepth(this, play)) ||
                // following a detour out of a tight spot: water a little shallow is fine, land isn't
                ((this->detourTimer > 0) && ((surface - floor) > this->halfHeight * 1.2f)))) {
        // stranded somewhere too shallow (released or dragged there): anywhere deeper is better
        this->waterY = surface;
        this->floorY = floor;
#if FISHMOD_OPT >= 5
        this->waterCell = -2;
#endif
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
        // slow down, but not to a stop, or it could never get away from the bank
        this->speed = CLAMP_MIN(this->speed * 0.5f, 0.4f);
        moved = false;
    }

    this->actor.shape.rot.x = CLAMP(-(s16)(dy * 300.0f), -0x1800, 0x1800);
    this->blocked = !moved;
    return moved;
}
#endif

#if FISHMOD_DEBUG
static s32 EnFishmod_SwimToward(EnFishmod* this, PlayState* play, Vec3f* target, f32 speed, s16 turnStep) {
    s32 ret;

    FISHMOD_PROF(FISHMOD_PROF_U_STEER, ret = EnFishmod_SwimTowardImpl(this, play, target, speed, turnStep));
    return ret;
}
#else
#define EnFishmod_SwimToward EnFishmod_SwimTowardImpl
#endif

static void EnFishmod_CheckSpookImpl(EnFishmod* this, PlayState* play) {
    FishmodRod* rod = &gFishmodRodState;
    Player* player = GET_PLAYER(play);
    f32 warySq = 30.0f + this->species->behavior.wariness * 0.7f;

    if (rod->splashed && (Math_Vec3f_DistXZ(&this->actor.world.pos, &rod->bobberPos) < warySq)) {
        EnFishmod_FishSetState(this, FISH_STATE_FLEE);
        return;
    }
    // Link: wading in nearby, running along the bank, or simply too close (nothing lets itself be stepped on)
    {
        f32 dist = Math_Vec3f_DistXZ(&this->actor.world.pos, &player->actor.world.pos);
        f32 calm = 20.0f + this->species->behavior.wariness * 0.6f + this->halfLength;
        s32 scared = (dist < this->halfLength + 25.0f) ||
                     ((player->actor.depthInWater > 0.0f) && (dist < calm * 1.3f)) ||
                     ((player->actor.speed > 4.0f) && (dist < calm));

        if (scared) {
            EnFishmod_FishSetState(this, FISH_STATE_FLEE);
            this->runYaw = Math_Vec3f_Yaw(&player->actor.world.pos, &this->actor.world.pos);
            this->fleeFromLink = true;
        }
    }
}

#if FISHMOD_DEBUG
static void EnFishmod_CheckSpook(EnFishmod* this, PlayState* play) {
    FISHMOD_PROF(FISHMOD_PROF_U_SPOOK, EnFishmod_CheckSpookImpl(this, play));
}
#else
#define EnFishmod_CheckSpook EnFishmod_CheckSpookImpl
#endif

/** True if there's water deep enough for this fish all the way from it to `to` (it can see and reach it). */
static s32 EnFishmod_OpenWaterToImpl(EnFishmod* this, PlayState* play, Vec3f* to) {
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

// where a fish takes the bait: the bait on its line under the bobber (FISHMOD_OPT 5), or just under the bobber
#if FISHMOD_OPT >= 5
#define FISHMOD_BAIT_MOUTH_Y(rod) FISHMOD_ROD_BAIT_Y(rod)
#else
#define FISHMOD_BAIT_MOUTH_Y(rod) ((rod)->bobberPos.y - 4.0f)
#endif

#if FISHMOD_DEBUG

static s32 EnFishmod_OpenWaterTo(EnFishmod* this, PlayState* play, Vec3f* to) {
    s32 ret;

    FISHMOD_PROF(FISHMOD_PROF_U_SEE, ret = EnFishmod_OpenWaterToImpl(this, play, to));
    return ret;
}
#else
#define EnFishmod_OpenWaterTo EnFishmod_OpenWaterToImpl
#endif

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

/**
 * A watchdog for fish that stop getting anywhere (wedged against a bank or in a corner the steering keeps turning
 * back into): every 40 frames, if a swimming fish has moved less than 4 units, it picks somewhere deep it can see
 * nearby and heads there for a while. If that doesn't free it either, it is nudged straight there.
 * Returns true while it is busy getting unstuck (the normal AI waits).
 */
static s32 EnFishmod_UnstickImpl(EnFishmod* this, PlayState* play) {
    s32 swimming = (this->fishState == FISH_STATE_WANDER) || (this->fishState == FISH_STATE_APPROACH) ||
                   (this->fishState == FISH_STATE_FLEE);

#if FISHMOD_OPT >= 5
    // a fish resting on purpose isn't stuck (the watchdog used to send resting fish off on escapes)
    if ((this->fishState == FISH_STATE_WANDER) && (this->restTimer > 0) && (this->escapeTimer <= 0)) {
        swimming = false;
    }
#endif

    if (!swimming) {
        this->stuckTimer = 0;
        this->stuckLevel = 0;
        this->escapeTimer = 0;
        this->stuckPos = this->actor.world.pos;
        return false;
    }

    if (++this->stuckTimer >= 40) {
        if (Math_Vec3f_DistXZ(&this->actor.world.pos, &this->stuckPos) < 4.0f) {
            f32 surface;
            f32 floor;
            s32 i;

            this->stuckLevel = CLAMP_MAX(this->stuckLevel + 1, 3);
            for (i = 0; i < 4; i++) {
                if (EnFishmod_FindWaterPoint(play, &this->actor.world.pos, 40.0f + i * 20.0f,
                                             this->halfHeight * 2.0f + 4.0f, &this->escape, &surface, &floor) &&
                    ((this->stuckLevel >= 2) || EnFishmod_OpenWaterTo(this, play, &this->escape))) {
                    this->escapeTimer = 60;
                    break;
                }
            }
        } else {
            this->stuckLevel = 0;
        }
        this->stuckPos = this->actor.world.pos;
        this->stuckTimer = 0;
    }

    if (this->escapeTimer <= 0) {
        return false;
    }
    this->escapeTimer--;
    if (this->stuckLevel >= 2) {
        // the steering keeps failing: go straight there, slowly
        f32 dist = Math_Vec3f_DistXZ(&this->actor.world.pos, &this->escape);
        s16 yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->escape);

        if (dist > 1.0f) {
            this->actor.world.pos.x += Math_SinS(yaw) * 0.8f;
            this->actor.world.pos.z += Math_CosS(yaw) * 0.8f;
        }
        Math_ApproachF(&this->actor.world.pos.y, this->escape.y, 0.1f, 0.5f);
        Math_SmoothStepToS(&this->actor.shape.rot.y, yaw, 3, 0x800, 0x100);
        this->speed = 0.8f;
    } else {
        EnFishmod_SwimToward(this, play, &this->escape, 0.8f, 0x800);
    }
    this->swimPhaseStep = 0.4f;
    this->swimMag = 1500.0f;
    if (this->escapeTimer == 0) {
        // free again: start over from here (an approach starts over too, later)
        this->target = this->escape;
        if (this->fishState == FISH_STATE_APPROACH) {
            this->cooldown = 200;
            EnFishmod_FishSetState(this, FISH_STATE_WANDER);
        }
    }
    return true;
}

#if FISHMOD_DEBUG
static s32 EnFishmod_Unstick(EnFishmod* this, PlayState* play) {
    s32 ret;

    FISHMOD_PROF(FISHMOD_PROF_U_UNSTICK, ret = EnFishmod_UnstickImpl(this, play));
    return ret;
}
#else
#define EnFishmod_Unstick EnFishmod_UnstickImpl
#endif

/**
 * A chunk of broken beehive (with larvae in it): tumbles onto the ground and lies there for half a minute. Swinging
 * an empty bottle at it scoops it up (Player's bottle catch list has it), like bugs.
 */
static void EnFishmod_CombUpdate(EnFishmod* this, PlayState* play) {
    if (this->actor.parent != NULL) {
        Actor_Kill(&this->actor); // in a bottle
        return;
    }
    if (--this->stateTimer <= 0) {
        Actor_Kill(&this->actor);
        return;
    }
    Actor_MoveXZGravity(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 8.0f, 8.0f, 8.0f,
                            UPDBGCHECKINFO_FLAG_0 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_3 |
                                UPDBGCHECKINFO_FLAG_4);
    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
        Math_ApproachZeroF(&this->actor.speed, 1.0f, 0.3f);
        Math_SmoothStepToS(&this->actor.shape.rot.x, 0x3400, 3, 0x800, 0x80); // lies tipped on its side
    } else {
        this->actor.shape.rot.x += 0x1200; // tumbling down
    }
    // in water it sinks slowly
    if (this->actor.bgCheckFlags & BGCHECKFLAG_WATER) {
        this->actor.velocity.y = CLAMP_MIN(this->actor.velocity.y, -0.5f);
    }
    Actor_OfferGetItem(&this->actor, play, GI_MAX, 60.0f, 30.0f);
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
#if FISHMOD_OPT >= 5
    // where the bait hangs: the float depth set on this cast
    below.y = FISHMOD_ROD_BAIT_Y(rod) - 2.0f;
#else
    below.y = rod->waterY - 10.0f;
#endif

    if (EnFishmod_Unstick(this, play)) {
        return;
    }

    switch (this->fishState) {
        case FISH_STATE_WANDER:
            // Fish don't roam about at random: they hold station near home for a while (resting, fins
            // working), then make a short cruise somewhere nearby (sometimes down to pick at the bottom), and
            // settle again. Fish out of their active time of day rest longer.
            if (this->restTimer > 0) {
                this->restTimer--;
                // holding the spot it stopped at (the target was set there), gliding to a stop
                EnFishmod_SwimToward(this, play, &this->target, 0.15f, 0x100);
#if FISHMOD_OPT >= 5
                // still alive while it rests: fins working, a slow hover up and down
                this->swimPhaseStep = 0.1f;
                this->swimMag = 650.0f;
                this->target.y += Math_SinS((s16)(this->timer * 0x300 + this->actor.params * 0x1234)) * 0.08f;
#else
                this->swimPhaseStep = 0.06f;
                this->swimMag = 450.0f;
#endif
                if (this->restTimer == 0) {
                    EnFishmod_PickWanderTarget(this, play);
                }
            } else {
                if ((this->stateTimer == 0) || (Math_Vec3f_DistXYZ(&this->actor.world.pos, &this->target) < 8.0f)) {
#if FISHMOD_OPT >= 5
                    // the more active its time of day, the shorter its rests (about 1.5 to 12 seconds)
                    this->restTimer = 30 + (s16)Rand_ZeroFloat(220.0f - EnFishmod_Activity(this) * 55.0f);
#else
                    this->restTimer = 40 + (s16)Rand_ZeroFloat(EnFishmod_ActiveNow(this) ? 120.0f : 300.0f);
#endif
                    this->target = this->actor.world.pos;
                }
                EnFishmod_SwimToward(this, play, &this->target, 0.4f + b->speed * 0.008f, 0x200);
                this->swimPhaseStep = 0.15f + this->speed * 0.12f;
                this->swimMag = 1300.0f;
            }

            EnFishmod_CheckSpook(this, play);
            // Only one fish goes to look at a time (not while another is at the bobber), and not often: a bobber
            // is something new in the water, not food falling from the sky.
            if ((this->fishState == FISH_STATE_WANDER) && (this->cooldown == 0) && EnFishmod_RodBobberInWater() &&
                (rod->fish == NULL) && (((play->gameplayFrames + this->actor.params) % 10) == 0)) {
                f32 notice = 30.0f + b->curiosity * 1.0f;

                if (rod->twitchTimer > 0) {
                    notice += 30.0f;
                }
#if FISHMOD_OPT >= 5
                // it has to be able to see the bait: not too far above or below it either (deep fish need the bait
                // brought down to them, or to have come up to feed)
                if (ABS(FISHMOD_ROD_BAIT_Y(rod) - this->actor.world.pos.y) > notice * 1.2f) {
                    notice = 0.0f;
                }
#endif
                if ((bobberDist < notice) && (Rand_ZeroOne() < b->curiosity * 0.0015f * EnFishmod_BaitAppeal(this)) &&
                    EnFishmod_OpenWaterTo(this, play, &rod->bobberPos)) {
                    this->fishState = FISH_STATE_APPROACH;
                    this->stateTimer = 200;
#if FISHMOD_OPT >= 5
                    // it noticed: a flick of the tail toward the bobber (the player sees it take interest)
                    this->startle = ((s16)(Math_Vec3f_Yaw(&this->actor.world.pos, &rod->bobberPos) -
                                           this->actor.shape.rot.y) > 0) ? 6 : -6;
                    // a fish deep below has further to come: time for the climb too
                    this->stateTimer += (s16)CLAMP(ABS(rod->waterY - this->actor.world.pos.y) * 2.5f, 0, 1000);
#endif
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
#if FISHMOD_OPT >= 5
            // now and then it backs off a little without turning, then comes back in (vanilla's pond fish do
            // this: it reads as the fish making up its mind)
            if (((this->timer + this->depthPref) % 90) < 22) {
                this->actor.world.pos.x -= Math_SinS(this->runYaw) * 0.3f;
                this->actor.world.pos.z -= Math_CosS(this->runYaw) * 0.3f;
                this->speed = 0.0f;
            } else {
                EnFishmod_SwimToward(this, play, &hover, 0.25f, 0x200);
            }
#else
            EnFishmod_SwimToward(this, play, &hover, 0.25f, 0x200);
#endif
            Math_SmoothStepToS(&this->actor.shape.rot.y, this->runYaw, 8, 0x100, 0x20);
            this->swimPhaseStep = 0.12f;
            EnFishmod_CheckSpook(this, play);
            if (this->fishState != FISH_STATE_INSPECT) {
                break;
            }

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
#if FISHMOD_OPT >= 5
                    // a wind-up first: it draws back, then strikes (the bite comes when it reaches the bait)
                    this->stateTimer = 14;
                    this->biteStruck = false;
#else
                    this->stateTimer = 0;
                    EnFishmod_RodBite(this);
#endif
                } else {
                    // lost interest
                    this->cooldown = 500 + (s16)Rand_ZeroFloat(300.0f);
#if FISHMOD_OPT >= 5
                    // a refusal the player can see: a sharp turn and a short dart away
                    EnFishmod_FishSetState(this, FISH_STATE_FLEE);
                    this->stateTimer = 22;
                    this->cooldown = 500 + (s16)Rand_ZeroFloat(300.0f);
#else
                    EnFishmod_FishSetState(this, FISH_STATE_WANDER);
#endif
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
#if FISHMOD_OPT >= 5
                EnFishmod_BobberStation(this, 0.45f, FISHMOD_ROD_BAIT_Y(rod), &peck);
#else
                EnFishmod_BobberStation(this, 0.45f, rod->waterY - 4.0f, &peck);
#endif
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
#if FISHMOD_OPT >= 5
            if (!this->biteStruck) {
                if (this->stateTimer > 7) {
                    // drawing back
                    this->actor.world.pos.x -= Math_SinS(this->runYaw) * 0.45f;
                    this->actor.world.pos.z -= Math_CosS(this->runYaw) * 0.45f;
                    this->swimPhaseStep = 0.3f;
                    Math_SmoothStepToS(&this->actor.shape.rot.y, this->runYaw, 8, 0x100, 0x20);
                    break;
                }
                // the strike: fast, straight at it
                EnFishmod_BobberStation(this, 0.5f, FISHMOD_BAIT_MOUTH_Y(rod), &mouth);
                EnFishmod_SwimToward(this, play, &mouth, 2.4f, 0x600);
                this->swimPhaseStep = 0.7f;
                this->jawRot = 6000;
                if ((this->stateTimer == 0) ||
                    (Math_Vec3f_DistXZ(&this->actor.world.pos, &mouth) < 3.0f + this->halfLength * 0.2f)) {
                    this->biteStruck = true;
                    EnFishmod_RodBite(this);
                }
                break;
            }
#endif
            // takes the bait and pulls the bobber down
            EnFishmod_BobberStation(this, 0.5f, FISHMOD_BAIT_MOUTH_Y(rod), &mouth);
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
            // Runs come in three kinds so the fight has a rhythm: off away from Link, a sweep across the line
            // (left or right, swinging the rod over), or, while still fresh, a charge back toward Link that throws
            // slack into the line.
            if (--this->runTimer <= 0) {
                s16 awayYaw = Math_Vec3f_Yaw(&player->actor.world.pos, &this->actor.world.pos);
                f32 fresh = this->stamina / this->staminaMax;
                f32 roll = Rand_ZeroOne();
                s16 yaw;

                if ((fresh > 0.5f) && (roll < 0.15f)) {
                    this->runKind = FISH_RUN_CHARGE;
                    yaw = awayYaw + 0x8000 + (s16)Rand_CenteredFloat(0x3000);
                } else if (roll < 0.55f) {
                    this->runKind = FISH_RUN_ACROSS;
                    yaw = awayYaw + ((Rand_ZeroOne() < 0.5f) ? 0x3800 : -0x3800) + (s16)Rand_CenteredFloat(0x1000);
                } else {
                    this->runKind = FISH_RUN_AWAY;
                    yaw = awayYaw + (s16)Rand_CenteredFloat(0x3000);
                }
                this->runYaw = EnFishmod_OpenWaterYaw(this, play, yaw);
                this->runTimer = 40 + (s16)Rand_ZeroFloat(70.0f);
                this->stateTimer = (s16)((12.0f + b->strength * 0.3f) * (0.3f + 0.7f * fresh));
                if (this->runKind == FISH_RUN_CHARGE) {
                    this->stateTimer = this->stateTimer * 2 / 3;
                }
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
                    // up near the surface while it runs, sounding deep while it holds against the line
                    f32 depthY = (this->stateTimer > 0)
                                     ? this->waterY - 8.0f
                                     : CLAMP_MIN(this->waterY - 8.0f - (this->waterY - this->floorY) * 0.5f,
                                                 this->floorY + this->halfHeight + 2.0f);

                    next.y = this->actor.world.pos.y;
                    Math_ApproachF(&next.y, depthY, 0.1f, 1.0f);
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
#if FISHMOD_OPT >= 5
                    this->waterCell = -2;
#endif
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
            // Hanging by the mouth from Link's right hand: move the fish so its head (measured on the last
            // draw, so it's right for every species and size) ends up at the hand.
            Vec3f* hand = &player->bodyPartsPos[PLAYER_BODYPART_R_HAND];
            Vec3f off;

            off.x = hand->x - this->headPos.x;
            off.y = hand->y - 2.0f - this->headPos.y;
            off.z = hand->z - this->headPos.z;
            // a big fish held by a small Link rests its tail on the ground rather than going into it
            off.y = CLAMP_MIN(off.y, player->actor.world.pos.y + 1.0f - this->boundsMin.y);
            if ((SQ(off.x) + SQ(off.y) + SQ(off.z)) > SQ(80.0f)) {
                // just landed: start with the body hanging under the hand
                this->actor.world.pos = *hand;
                this->actor.world.pos.y -= this->halfLength;
            } else {
                this->actor.world.pos.x += off.x * 0.6f;
                this->actor.world.pos.y += off.y * 0.6f;
                this->actor.world.pos.z += off.z * 0.6f;
            }
            // head up, swinging and twisting on the line
            this->actor.shape.rot.y = player->actor.shape.rot.y + 0x4000 + (s16)(Math_SinS(this->timer * 0x900) * 0x1800);
            this->actor.shape.rot.x = -0x4000 + (s16)(Math_SinS(this->timer * 0x700) * 0x500);
            this->actor.shape.rot.z = 0;
            this->swimPhaseStep = 0.6f;
            this->swimMag = 2500.0f;
            break;
        }

        case FISH_STATE_FLEE: {
            Vec3f away;

            // pick a way out once (away from the bobber, toward open water), and again only if the shore is
            // in the way
            if (this->stateTimer == 49) {
                this->runYaw = EnFishmod_OpenWaterYaw(
                    this, play,
                    this->fleeFromLink ? this->runYaw : Math_Vec3f_Yaw(&rod->bobberPos, &this->actor.world.pos));
                this->fleeFromLink = false;
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
            if (gFishmodStudio.active) {
                // the animation studio moves it
                this->actor.shape.rot.y = gFishmodStudio.yaw;
                this->actor.shape.rot.x = gFishmodStudio.pitch;
                this->actor.shape.rot.z = gFishmodStudio.roll;
                this->swimMag = gFishmodStudio.swimMag;
                this->swimPhaseStep = gFishmodStudio.swimPhaseStep;
                this->jawRot = gFishmodStudio.jawRot;
                if (gFishmodStudio.startle != 0) {
                    this->startle = gFishmodStudio.startle;
                }
                break;
            }
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

    // Turning bends the whole body into the turn (each joint adds to the one before, so the tail sweeps wide),
    // and a startle throws it into a sharp C for a moment before it shoots off.
    {
        s16 turn = this->actor.shape.rot.y - this->lastYaw;
        s16 bendTarget = CLAMP(turn * 5, -0x1400, 0x1400);

        this->lastYaw = this->actor.shape.rot.y;
        Math_SmoothStepToS(&this->turnBend, bendTarget, 3, 0x600, 0x40);
    }
    // traveling wave down the body, stronger toward the tail
    for (i = 0; i < 4; i++) {
#if FISHMOD_OPT >= 4
        // the sine table instead of cosf: the same wave to well under a degree
        this->bodyYaw[i] = Math_CosS((s16)((this->swimPhase - (i * 1.1f)) * (0x8000 / M_PI))) * this->swimMag *
                               (0.6f + (i * 0.55f)) -
                           this->turnBend * (0.25f + i * 0.2f);
#else
        this->bodyYaw[i] = cosf(this->swimPhase - (i * 1.1f)) * this->swimMag * (0.6f + (i * 0.55f)) -
                           this->turnBend * (0.25f + i * 0.2f);
#endif
    }
    if (this->startle != 0) {
        s32 dir = (this->startle > 0) ? 1 : -1;
        s32 left = ABS(this->startle);

        for (i = 0; i < 4; i++) {
            this->bodyYaw[i] += dir * 0x900 * (left > 4 ? 1.0f : -0.6f) * (0.5f + i * 0.3f);
        }
        this->startleBend = dir * 0x900 * (left > 4 ? 1.0f : -0.6f);
        this->startle -= dir;
    } else {
        this->startleBend = 0;
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

/** A frame without the AI: keep going the way it was headed. */
static void EnFishmod_Glide(EnFishmod* this) {
    this->actor.world.pos.x += Math_SinS(this->actor.shape.rot.y) * this->speed;
    this->actor.world.pos.z += Math_CosS(this->actor.shape.rot.y) * this->speed;
    if (this->stateTimer > 0) {
        this->stateTimer--;
    }
#if FISHMOD_OPT >= 5
    // its rest runs on whether it thinks this frame or not (else a far fish rests many times as long, and is still
    // resting, frozen, when the camera comes close)
    if (this->restTimer > 1) {
        this->restTimer--;
    }
#endif
    if (this->cooldown > 0) {
        this->cooldown--;
    }
    EnFishmod_UpdateSwim(this);
}

void EnFishmod_Update(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;

    this->timer++;
#if FISHMOD_OPT >= 4
    if (this->type == FISHMOD_TYPE_MANAGER) {
        EnFishmod_ManagerUpdate(this, play);
        return;
    }
#endif

    if (this->type == FISHMOD_TYPE_ROD) {
        EnFishmod_RodUpdate(this, play);
        gFishmodRodState.splashed = false;
        return;
    }
    if (this->type == FISHMOD_TYPE_COMB) {
        EnFishmod_CombUpdate(this, play);
        return;
    }

#if FISHMOD_OPT >= 3
    // Away from attention (FISHMOD_OPT 5: the camera or the bobber, whichever is nearer; before: Link) a fish does
    // less: past 500 units it thinks every 2nd frame, past 800 every 4th, past 1200 every 8th (gliding on in
    // between), and past 1600 it goes dormant: it holds still (and is out of drawing range) until attention comes.
    if ((this->fishState == FISH_STATE_WANDER) && !gFishmodPhoto.active) {
        f32 attnDist = EnFishmod_AttentionDist(this, play);

        if (attnDist > 1600.0f) {
            return;
        }
        this->thinkMask = (attnDist > 1200.0f) ? 7 : (attnDist > 800.0f) ? 3 : (attnDist > 500.0f) ? 1 : 0;
    } else {
        this->thinkMask = 0;
    }
#endif
#if FISHMOD_OPT >= 2
    // away from attention, a wandering fish thinks every other frame and glides on in between
    if ((this->fishState == FISH_STATE_WANDER) && (this->timer & FISHMOD_THINK_MASK(this)) && !gFishmodPhoto.active &&
        (EnFishmod_AttentionDist(this, play) > 500.0f)) {
        FISHMOD_PROF(FISHMOD_PROF_U_GLIDE, EnFishmod_Glide(this));
        return;
    }
#endif
#if FISHMOD_OPT >= 4
    if ((this->type == FISHMOD_TYPE_FISH) && !EnFishmod_ThinkAllowed(this, play)) {
        FISHMOD_PROF(FISHMOD_PROF_U_GLIDE, EnFishmod_Glide(this));
        return;
    }
#endif
    FISHMOD_PROF(FISHMOD_PROF_U_AI, EnFishmod_FishAI(this, play));
    FISHMOD_PROF(FISHMOD_PROF_U_SWIM, EnFishmod_UpdateSwim(this));

#if FISHMOD_DEBUG
    {
    OSTime debugStart = osGetTime();
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
            Fishmod_DebugPrintf(6, " %d %d %d w%d f%d hh%d hl%d b%d", (s32)this->actor.world.pos.x, (s32)this->actor.world.pos.y, (s32)this->actor.world.pos.z, (s32)this->waterY, (s32)this->floorY, (s32)this->halfHeight, (s32)this->halfLength, this->blocked);
        }
    }
    gFishmodProf[FISHMOD_PROF_U_DEBUG] += osGetTime() - debugStart;
    gFishmodProfCount[FISHMOD_PROF_U_DEBUG]++;
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
        } else if (limbIndex == FISHMOD_BASS_JAW_LIMB + 1) {
            // shut at rest (the model's own pose gapes); jawRot opens it from there
            rot->z += FISHMOD_BASS_JAW_SHUT - this->jawRot;
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

#if FISHMOD_DEBUG
// the limb callbacks with profile timers around them (debug builds only)
static s32 EnFishmod_OverrideLimbDrawProf(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                          void* thisx) {
    s32 ret;

    FISHMOD_PROF(FISHMOD_PROF_D_LIMB_CB, ret = EnFishmod_OverrideLimbDraw(play, limbIndex, dList, pos, rot, thisx));
    return ret;
}

static void EnFishmod_PostLimbDrawProf(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx) {
    FISHMOD_PROF(FISHMOD_PROF_D_LIMB_CB, EnFishmod_PostLimbDraw(play, limbIndex, dList, rot, thisx));
}
#define FISHMOD_LIMB_OVERRIDE EnFishmod_OverrideLimbDrawProf
#define FISHMOD_LIMB_POST EnFishmod_PostLimbDrawProf
#else
#define FISHMOD_LIMB_OVERRIDE EnFishmod_OverrideLimbDraw
#define FISHMOD_LIMB_POST EnFishmod_PostLimbDraw
#endif

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
void EnFishmod_DrawPhotoBackdrop(PlayState* play) {
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

#if (FISHMOD_OPT >= 2) && (FISHMOD_OPT < 4)
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
                              this->skelAnime.dListCount, FISHMOD_LIMB_OVERRIDE, FISHMOD_LIMB_POST, this);
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

#if FISHMOD_OPT >= 5
/**
 * The far shape's material for a species (FISHMOD_OPT 5): its side texture (the scale pattern the model tiles along
 * the body) with its palette, coloured by the vertex colours. The manager sets it once per species.
 */
void EnFishmod_BillboardMaterial(EnFishmod* this, PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    s32 speciesIndex = this->species - gFishmodSpecies;
    u8* tex = sSpeciesTexturesCI[speciesIndex][FISHMOD_TEX_ROLE_SIDE];

    OPEN_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
    Gfx_SetupDL_25Opa(gfxCtx);
    gDPPipeSync(POLY_OPA_DISP++);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH | G_TEXTURE_GEN);
    if ((tex != NULL) && (sSpeciesPalettes[speciesIndex] != NULL)) {
        s32 big = (speciesIndex == FISHMOD_SPECIES_ORDON_CATFISH) || (speciesIndex == FISHMOD_SPECIES_HYLIAN_LOACH);

        gDPLoadTLUT_pal256(POLY_OPA_DISP++, sSpeciesPalettes[speciesIndex]);
        gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_RGBA16);
        if (big) {
            gDPLoadTextureBlock(POLY_OPA_DISP++, tex, G_IM_FMT_CI, G_IM_SIZ_8b, 32, 32, 0, G_TX_WRAP | G_TX_NOMIRROR,
                                G_TX_WRAP | G_TX_NOMIRROR, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPLoadTextureBlock(POLY_OPA_DISP++, tex, G_IM_FMT_CI, G_IM_SIZ_8b, 16, 16, 0, G_TX_WRAP | G_TX_NOMIRROR,
                                G_TX_WRAP | G_TX_NOMIRROR, 4, 4, G_TX_NOLOD, G_TX_NOLOD);
        }
        gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA);
    } else {
        gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_OPA_DISP++, G_CC_SHADE, G_CC_SHADE);
    }
    CLOSE_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
}
#endif

#if FISHMOD_OPT >= 3
/**
 * A distant fish: a flat hexagon (nose, back, tail, belly) turned about its long axis to face the camera. Before
 * FISHMOD_OPT 5 in the fish's own colour (vertex colours only); from 5 with its scale pattern tiled along it and
 * coloured like the model (head, back and belly colours), set by EnFishmod_BillboardMaterial.
 */
void EnFishmod_DrawBillboard(EnFishmod* this, PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Vtx* vtx = GRAPH_ALLOC(gfxCtx, 6 * sizeof(Vtx));
    f32 fx = Math_SinS(this->actor.shape.rot.y);
    f32 fz = Math_CosS(this->actor.shape.rot.y);
    f32 vx = this->actor.world.pos.x - play->view.eye.x;
    f32 vy = this->actor.world.pos.y - play->view.eye.y;
    f32 vz = this->actor.world.pos.z - play->view.eye.z;
#if FISHMOD_OPT >= 5
    // Facing the camera, its length foreshortened by the angle it's seen at (nose-on it's a short oval, never a line
    // seen edge-on, which made fish vanish from straight ahead or behind): along = F with the line of sight taken out
    f32 vl = sqrtf(SQ(vx) + SQ(vy) + SQ(vz));
    f32 ax;
    f32 ay;
    f32 az;
    f32 al;
    f32 fore;
    f32 ux;
    f32 uy;
    f32 uz;
    f32 ul;
    f32 hl = this->halfLength;
    f32 hh = this->halfHeight * 1.3f;

#define BILLBOARD_ALONG_Y (ay * fore)
#else
    // across the body, at right angles to both the fish and the line of sight: U = F x V, F = (fx, 0, fz)
    f32 ux = -fz * vy;
    f32 uy = fz * vx - fx * vz;
    f32 uz = fx * vy;
    f32 ul = sqrtf(SQ(ux) + SQ(uy) + SQ(uz));
    f32 hl = this->halfLength;
    f32 hh = this->halfHeight * 1.3f;
#define BILLBOARD_ALONG_Y 0.0f
#endif
    static const f32 sShape[6][2] = {
        { 1.0f, 0.0f }, { 0.3f, 1.0f }, { -0.5f, 0.7f }, { -1.0f, 0.0f }, { -0.5f, -0.7f }, { 0.3f, -1.0f },
    };
#if FISHMOD_OPT >= 5
    // nose, back (front), back (rear), tail, belly (rear), belly (front)
    static const u8 sPart[6] = { FISHMOD_PART_HEAD,      FISHMOD_PART_BODY_FRONT, FISHMOD_PART_BODY_REAR,
                                 FISHMOD_PART_TAIL,      FISHMOD_PART_FIN_BELLY,  FISHMOD_PART_BODY_FRONT };
    // the pattern tiled about twice along the body and once across, like on the model
    s32 texSize = ((this->species - gFishmodSpecies) == FISHMOD_SPECIES_ORDON_CATFISH) ||
                          ((this->species - gFishmodSpecies) == FISHMOD_SPECIES_HYLIAN_LOACH)
                      ? 32
                      : 16;
#endif
    const u8* tint = this->species->tint;
    s32 i;

#if FISHMOD_OPT >= 5
    if (vl < 0.001f) {
        vl = 0.001f;
    }
    vx /= vl;
    vy /= vl;
    vz /= vl;
    {
        f32 d = fx * vx + fz * vz; // F . V (F is level)

        ax = fx - d * vx;
        ay = -d * vy;
        az = fz - d * vz;
    }
    al = sqrtf(SQ(ax) + SQ(ay) + SQ(az)); // the sine of the angle between the fish and the line of sight
    if (al < 0.05f) {
        // dead ahead or behind: any direction across the view will do (the oval is round enough)
        ax = vz;
        ay = 0.0f;
        az = -vx;
        al = sqrtf(SQ(ax) + SQ(az)) + 0.0001f;
    }
    ax /= al;
    ay /= al;
    az /= al;
    fore = CLAMP(al, hh / CLAMP_MIN(hl, 1.0f), 1.0f);
    // across: at right angles to the length and the line of sight
    ux = ay * vz - az * vy;
    uy = az * vx - ax * vz;
    uz = ax * vy - ay * vx;
    ul = sqrtf(SQ(ux) + SQ(uy) + SQ(uz));
    // the drawing below uses (fx, 0, fz) as the length: the foreshortened direction instead
    fx = ax * fore;
    fz = az * fore;
#endif
    if (ul < 0.001f) {
        ux = 0.0f;
        uy = 1.0f;
        uz = 0.0f;
    } else {
        ux /= ul;
        uy /= ul;
        uz /= ul;
    }
    for (i = 0; i < 6; i++) {
        f32 along = sShape[i][0] * hl;
        f32 across = sShape[i][1] * hh;
        // a bit darker toward the back and tail, like the real thing seen through water
        u8 shade = (i == 3) ? 150 : (i == 0) ? 200 : 175;

        vtx[i].v.ob[0] = (s16)(fx * along + ux * across);
        vtx[i].v.ob[1] = (s16)(BILLBOARD_ALONG_Y * along + uy * across);
        vtx[i].v.ob[2] = (s16)(fz * along + uz * across);
        vtx[i].v.flag = 0;
#if FISHMOD_OPT >= 5
        {
            const u8* c = this->species->partTint[sPart[i]];

            // belly lighter, as on the model
            shade = (i >= 4) ? 235 : (i == 3) ? 170 : 205;
            vtx[i].v.tc[0] = (s16)((1.0f - sShape[i][0]) * texSize * 32.0f);
            vtx[i].v.tc[1] = (s16)((1.0f - sShape[i][1]) * 0.5f * texSize * 32.0f);
            vtx[i].v.cn[0] = c[0] * shade / 255;
            vtx[i].v.cn[1] = c[1] * shade / 255;
            vtx[i].v.cn[2] = c[2] * shade / 255;
        }
#else
        vtx[i].v.tc[0] = vtx[i].v.tc[1] = 0;
        vtx[i].v.cn[0] = tint[0] * shade / 255;
        vtx[i].v.cn[1] = tint[1] * shade / 255;
        vtx[i].v.cn[2] = tint[2] * shade / 255;
#endif
        vtx[i].v.cn[3] = 255;
    }

    OPEN_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
#if FISHMOD_OPT < 5
    Gfx_SetupDL_25Opa(gfxCtx);
    gDPPipeSync(POLY_OPA_DISP++);
    gSPTexture(POLY_OPA_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_SHADE, G_CC_SHADE);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH | G_TEXTURE_GEN);
#endif
    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx, "../z_en_fishmod.c", __LINE__);
    gSPVertex(POLY_OPA_DISP++, vtx, 6, 0);
    gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    gSP2Triangles(POLY_OPA_DISP++, 0, 3, 4, 0, 0, 4, 5, 0);
    CLOSE_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
}
#undef BILLBOARD_ALONG_Y
#endif

#if FISHMOD_OPT >= 4
/** A species' textures and palette behind their segments (the manager does this once per species). */
void EnFishmod_SetSpeciesTextures(EnFishmod* this, PlayState* play) {
    s32 speciesIndex = this->species - gFishmodSpecies;
    s32 role;
    IF_F3DEX3_DONT_SKIP_TEX_INIT();

    OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
    IF_F3DEX3_DONT_SKIP_TEX_HERE(POLY_OPA_DISP++, speciesIndex);
    if (sSpeciesPalettes[speciesIndex] != NULL) {
        gDPLoadTLUT_pal256(POLY_OPA_DISP++, sSpeciesPalettes[speciesIndex]);
    }
    for (role = 0; role < FISHMOD_TEX_ROLE_MAX; role++) {
        void* tex = sSpeciesTexturesCI[speciesIndex][role];

        if (tex != NULL) {
            gSPSegment(POLY_OPA_DISP++, 0x08 + role, tex);
        }
    }
    CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
}

/**
 * Whiskers fixed to the head (no sway): the same shapes and sizes as before, placed from the head's position and
 * the fish's facing, pitch and head swing. One matrix and one vertex load per fish.
 */
void EnFishmod_DrawWhiskersRigid(EnFishmod* this, PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const u8* tint = this->species->partTint[FISHMOD_PART_HEAD];
    Vtx* vtx = GRAPH_ALLOC(gfxCtx, 32 * sizeof(Vtx));
    f32 dx = this->boundsMax.x - this->boundsMin.x;
    f32 dz = this->boundsMax.z - this->boundsMin.z;
    f32 length = sqrtf(SQ(dx) + SQ(dz)) * 1.9f;
    Vec3f pts[4][3];
    f32 rot[3][3];
    s16 yaw = this->actor.shape.rot.y + this->bodyYaw[0] / 2;
    f32 sy = Math_SinS(yaw);
    f32 cy = Math_CosS(yaw);
    f32 sx = Math_SinS(this->actor.shape.rot.x);
    f32 cx = Math_CosS(this->actor.shape.rot.x);
    s32 i;
    s32 k;
    s32 side;
    s32 v = 0;

    // fish frame: Ry(yaw + head swing) * Rx(pitch); fish swim upright, so roll is left out
    rot[0][0] = cy;
    rot[0][1] = sy * sx;
    rot[0][2] = sy * cx;
    rot[1][0] = 0.0f;
    rot[1][1] = cx;
    rot[1][2] = -sx;
    rot[2][0] = -sy;
    rot[2][1] = cy * sx;
    rot[2][2] = cy * cx;

    for (i = 0; i < 2; i++) {
        for (k = 0; k < 3; k++) {
            Vec3f* p = &sWhiskerPoints[i][k];

            for (side = 0; side < 2; side++) {
                // fish space: x = side, y = up, z = forward
                f32 lx = ((side == 0) ? p->z : -p->z) * length;
                f32 ly = p->y * length;
                f32 lz = (p->x + 0.03f) * length;
                Vec3f* out = &pts[i * 2 + side][k];

                // relative to the head, in eighths of a unit (the vertices are s16)
                out->x = (rot[0][0] * lx + rot[0][1] * ly + rot[0][2] * lz) * 8.0f;
                out->y = (rot[1][0] * lx + rot[1][1] * ly + rot[1][2] * lz) * 8.0f;
                out->z = (rot[2][0] * lx + rot[2][1] * ly + rot[2][2] * lz) * 8.0f;
            }
        }
    }

    // each segment a flat, level ribbon like the fishing line model, as wide as before
    for (i = 0; i < 4; i++) {
        for (k = 0; k < 2; k++) {
            Vec3f* a = &pts[i][k];
            Vec3f* b = &pts[i][k + 1];
            f32 segX = b->x - a->x;
            f32 segZ = b->z - a->z;
            f32 distXZ = sqrtf(SQ(segX) + SQ(segZ));
            f32 half = 150.0f * this->actor.scale.z * (1.0f - k * 0.45f) * 8.0f;
            f32 wx = (distXZ > 0.01f) ? (segZ / distXZ) * half : half;
            f32 wz = (distXZ > 0.01f) ? (-segX / distXZ) * half : 0.0f;
            Vec3f c[4];
            s32 n;

            c[0].x = a->x - wx;
            c[0].y = a->y;
            c[0].z = a->z - wz;
            c[1].x = a->x + wx;
            c[1].y = a->y;
            c[1].z = a->z + wz;
            c[2].x = b->x + wx;
            c[2].y = b->y;
            c[2].z = b->z + wz;
            c[3].x = b->x - wx;
            c[3].y = b->y;
            c[3].z = b->z - wz;
            for (n = 0; n < 4; n++, v++) {
                vtx[v].v.ob[0] = c[n].x;
                vtx[v].v.ob[1] = c[n].y;
                vtx[v].v.ob[2] = c[n].z;
                vtx[v].v.flag = 0;
                vtx[v].v.tc[0] = vtx[v].v.tc[1] = 0;
                vtx[v].v.cn[0] = vtx[v].v.cn[1] = vtx[v].v.cn[2] = 0;
                vtx[v].v.cn[3] = 0xFF;
            }
        }
    }

    OPEN_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
    POLY_OPA_DISP = Gfx_SetupDL(POLY_OPA_DISP, SETUPDL_25);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, tint[0] / 2, tint[1] / 2, tint[2] / 2, 255);
    Matrix_Translate(this->headPos.x, this->headPos.y, this->headPos.z, MTXMODE_NEW);
    Matrix_Scale(1.0f / 8.0f, 1.0f / 8.0f, 1.0f / 8.0f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx, "../z_en_fishmod.c", __LINE__);
    gSPVertex(POLY_OPA_DISP++, vtx, 32, 0);
    for (i = 0; i < 32; i += 8) {
        gSP2Triangles(POLY_OPA_DISP++, i + 0, i + 1, i + 2, 0, i + 0, i + 2, i + 3, 0);
        gSP2Triangles(POLY_OPA_DISP++, i + 4, i + 5, i + 6, 0, i + 4, i + 6, i + 7, 0);
    }
    CLOSE_DISPS(gfxCtx, "../z_en_fishmod.c", __LINE__);
}

/** Photo mode: where the fish is and how big, for the camera. */
void EnFishmod_PhotoBounds(EnFishmod* this) {
    f32 dx = this->boundsMax.x - this->boundsMin.x;
    f32 dy = this->boundsMax.y - this->boundsMin.y;
    f32 dz = this->boundsMax.z - this->boundsMin.z;

    gFishmodPhoto.center.x = (this->boundsMin.x + this->boundsMax.x) * 0.5f;
    gFishmodPhoto.center.y = (this->boundsMin.y + this->boundsMax.y) * 0.5f;
    gFishmodPhoto.center.z = (this->boundsMin.z + this->boundsMax.z) * 0.5f;
    gFishmodPhoto.extent = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}
#endif

void EnFishmod_Draw(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;
    const u8* shape;
#if FISHMOD_DEBUG
    OSTime profStart;
#endif

    if (this->type == FISHMOD_TYPE_ROD) {
        EnFishmod_RodDraw(this, play);
        return;
    }
#if FISHMOD_OPT >= 4
    if (this->type == FISHMOD_TYPE_MANAGER) {
        EnFishmod_ManagerDraw(this, play);
        return;
    }
#endif
#if FISHMOD_OPT >= 3
    // far from the camera, a flat shape in the fish's colour instead of the textured skeleton
    if ((this->type == FISHMOD_TYPE_FISH) && !gFishmodPhoto.active && (this->fishState != FISH_STATE_CAUGHT) &&
        (Math_Vec3f_DistXYZ(&this->actor.world.pos, &play->view.eye) > FISHMOD_LOD_DIST)) {
#if FISHMOD_OPT >= 5
        EnFishmod_BillboardMaterial(this, play);
#endif
        FISHMOD_PROF(FISHMOD_PROF_D_BILLBOARD, EnFishmod_DrawBillboard(this, play));
        return;
    }
#endif
    if (this->type == FISHMOD_TYPE_COMB) {
        // flickers for its last few seconds
        if ((this->stateTimer > 60) || (this->timer & 2)) {
            OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            // turned about its middle (the hive model stands 236 units tall from its origin), like Obj_Comb does
            Matrix_SetTranslateRotateYXZ(this->actor.world.pos.x, this->actor.world.pos.y + 118.0f * this->actor.scale.y,
                                         this->actor.world.pos.z, &this->actor.shape.rot);
            Matrix_Translate(0.0f, -118.0f * this->actor.scale.y, 0.0f, MTXMODE_APPLY);
            Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
            gSPDisplayList(POLY_OPA_DISP++, gFieldBeehiveDL);
            CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
        }
        return;
    }

    if (gFishmodPhoto.active && (gFishmodPhoto.backdropFrame != play->gameplayFrames)) {
        gFishmodPhoto.backdropFrame = play->gameplayFrames;
        EnFishmod_DrawPhotoBackdrop(play);
    }

#if FISHMOD_DEBUG
    profStart = osGetTime();
#endif
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
#if FISHMOD_DEBUG
    gFishmodProf[FISHMOD_PROF_D_SETUP] += osGetTime() - profStart;
    gFishmodProfCount[FISHMOD_PROF_D_SETUP]++;
    profStart = osGetTime();
#endif

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
#if FISHMOD_DEBUG
    gFishmodProf[FISHMOD_PROF_D_ROOT] += osGetTime() - profStart;
    gFishmodProfCount[FISHMOD_PROF_D_ROOT]++;
#endif
#if (FISHMOD_OPT >= 2) && (FISHMOD_OPT < 4)
    FISHMOD_PROF(FISHMOD_PROF_D_POSE, EnFishmod_DrawPose(this, play));
#else
#if FISHMOD_OPT >= 5
    // The bass's display lists don't set culling (EnFishmod_PatchSkeleton): with its mouth open you look at the
    // inside of the head and body, their back faces, so it draws both sides then; shut, it culls them
    if (this->species->base == FISHMOD_BASE_BASS) {
        OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
        if (this->jawRot > 0) {
            gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BACK);
        } else {
            gSPSetGeometryMode(POLY_OPA_DISP++, G_CULL_BACK);
        }
        CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
    }
#endif
    FISHMOD_PROF(FISHMOD_PROF_D_POSE,
                 SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable,
                                       this->skelAnime.dListCount, FISHMOD_LIMB_OVERRIDE, FISHMOD_LIMB_POST, this));
#if FISHMOD_OPT >= 5
    if (this->species->base == FISHMOD_BASE_BASS) {
        OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
        gSPSetGeometryMode(POLY_OPA_DISP++, G_CULL_BACK);
        CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
    }
#endif
#endif

    if (this->species->lookFlags & FISHMOD_LOOK_WHISKERS) {
        FISHMOD_PROF(FISHMOD_PROF_D_WHISKERS,
                     (EnFishmod_UpdateWhiskers(this), EnFishmod_DrawWhiskers(this, play)));
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
