/*
 * File: z_en_fishmod.c
 * Overlay: ovl_En_Fishmod
 * Description: fishmod overworld fishing. Type FISHMOD_TYPE_FISH is one fish of a species from
 * fishmod/data/species.json, drawn by reshaping and recoloring the Fishing Pond bass and loach models.
 * Type FISHMOD_TYPE_ROD is the per-scene fishing controller (rod, line, bobber, camera, spawning).
 */

#include "z_en_fishmod.h"

#include "assets/objects/object_fish/object_fish.h"
#include "array_count.h"
#include "libc64/math64.h"
#include "gfx.h"
#include "gfx_setupdl.h"
#include "ichain.h"
#include "rand.h"
#include "segmented_address.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "play_state.h"
#include "player.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

void EnFishmod_Init(Actor* thisx, PlayState* play);
void EnFishmod_Destroy(Actor* thisx, PlayState* play);
void EnFishmod_Update(Actor* thisx, PlayState* play);
void EnFishmod_Draw(Actor* thisx, PlayState* play);

void EnFishmod_RodIdle(EnFishmod* this, PlayState* play);
void EnFishmod_FishHover(EnFishmod* this, PlayState* play);

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

#include "fishmod_textures.inc.c"

// Pond textures for each role (back, belly, head, side, fin); must match BASE_TEXTURES in gen_textures.py
static void* sTextureRoles[2][FISHMOD_TEX_ROLE_MAX] = {
    // the bass has no side texture, so its "side" slot is the tail fin
    { gFishingFishTex_0016D0, gFishingFishTex_0018D0, gFishingFishTex_001AD0, gFishingFishTex_0014D0,
      gFishingFishTex_0022D0 },
    { gFishingLoachTex_00ED28, gFishingLoachTex_00F528, gFishingLoachTex_010528, gFishingLoachTex_00FD28,
      gFishingLoachTex_010DA8 },
};

/**
 * The pond models set their own primitive color (the tint) in every limb display list. Remove those
 * commands from the copy of object_fish in RAM so each fish can set its own tint per limb.
 * Idempotent, so it is safe to run for every fish.
 */
static void EnFishmod_PatchSkeleton(FlexSkeletonHeader* skelHeader, s32 isLoach) {
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
                    if ((sTextureRoles[isLoach][role] != NULL) &&
                        (gfx->words.w1 == (uintptr_t)sTextureRoles[isLoach][role])) {
                        gfx->words.w1 = (u32)(0x08 + role) << 24;
                        break;
                    }
                }
            }
        }
    }
}

static void EnFishmod_InitLimbData(EnFishmod* this, FlexSkeletonHeader* skelHeader) {
    void** limbs = SEGMENTED_TO_VIRTUAL(skelHeader->sh.segment);
    u8* partMap = (this->species->base == FISHMOD_BASE_LOACH) ? sLoachLimbParts : sBassLimbParts;
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
}

void EnFishmod_SetLength(EnFishmod* this, f32 lengthInches) {
    f32 cm = lengthInches * 2.54f;

    this->lengthInches = lengthInches;
    if (this->species->base == FISHMOD_BASE_LOACH) {
        Actor_SetScale(&this->actor, cm * 65.0f * 0.000001f);
        this->actor.scale.x = this->actor.scale.y = this->actor.scale.z * 1.1f;
    } else {
        Actor_SetScale(&this->actor, cm * 15.0f * 0.00001f);
    }
}

void EnFishmod_Init(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;
    FlexSkeletonHeader* skel;
    AnimationHeader* anim;
    s32 species;

    this->type = FISHMOD_GET_TYPE(thisx->params);

    if (this->type == FISHMOD_TYPE_ROD) {
        thisx->draw = NULL;
        this->actionFunc = EnFishmod_RodIdle;
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
    } else {
        skel = &gFishingFishSkel;
        anim = &gFishingFishAnim;
    }

    SkelAnime_InitFlex(play, &this->skelAnime, skel, anim, this->jointTable, this->morphTable,
                       EN_FISHMOD_MAX_LIMBS + 1);
    Animation_MorphToLoop(&this->skelAnime, anim, 0.0f);

    EnFishmod_PatchSkeleton(SEGMENTED_TO_VIRTUAL(skel), this->species->base == FISHMOD_BASE_LOACH);
    EnFishmod_InitLimbData(this, SEGMENTED_TO_VIRTUAL(skel));

    EnFishmod_SetLength(this, this->species->lengthTypical);
    this->swimPhaseStep = 0.25f;
    this->swimMag = 1500.0f;
    this->swimPhase = Rand_ZeroFloat(6.28f);
    this->actionFunc = EnFishmod_FishHover;
}

void EnFishmod_Destroy(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;

    if (this->type == FISHMOD_TYPE_ROD) {
        if (gFishmodRod == thisx) {
            gFishmodRod = NULL;
        }
    } else {
        SkelAnime_Free(&this->skelAnime, play);
    }
}

void EnFishmod_RodIdle(EnFishmod* this, PlayState* play) {
}

/**
 * Holds position with a gentle swimming motion (used until the fish AI is in, and for photos).
 */
void EnFishmod_FishHover(EnFishmod* this, PlayState* play) {
    if (gFishmodPhoto.active) {
        this->actor.world.pos = this->actor.home.pos;
        this->actor.shape.rot.y = gFishmodPhoto.yaw;
        this->actor.shape.rot.x = gFishmodPhoto.pitch;
        // a gentle, nearly straight pose for portraits
        this->swimMag = 250.0f;
        this->swimPhaseStep = 0.08f;
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

void EnFishmod_Update(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;

    this->timer++;
    this->actionFunc(this, play);

    if (this->type == FISHMOD_TYPE_FISH) {
        EnFishmod_UpdateSwim(this);
    }
}

s32 EnFishmod_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx) {
    EnFishmod* this = (EnFishmod*)thisx;
    s32 li = limbIndex - 1;
    u8* partMap = (this->species->base == FISHMOD_BASE_LOACH) ? sLoachLimbParts : sBassLimbParts;
    u8 part;
    Vec3f* scale;
    Vec3f parentScale;

    if (li >= this->limbCount) {
        return false;
    }

    // swimming
    if (this->species->base == FISHMOD_BASE_LOACH) {
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
    if (part != FISHMOD_PART_NONE) {
        const u8* tint = this->species->partTint[part];

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

    if ((parentScale.x != 1.0f) || (parentScale.y != 1.0f) || (parentScale.z != 1.0f) || (scale->x != 1.0f) ||
        (scale->y != 1.0f) || (scale->z != 1.0f)) {
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
    f32 length = sqrtf(SQ(dx) + SQ(dz)) * 1.2f;
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

void EnFishmod_Draw(Actor* thisx, PlayState* play) {
    EnFishmod* this = (EnFishmod*)thisx;
    const u8* shape;

    if (gFishmodPhoto.active) {
        EnFishmod_DrawPhotoBackdrop(play);
    }

    Gfx_SetupDL_25Opa(play->state.gfxCtx);

    {
        s32 isLoach = (this->species->base == FISHMOD_BASE_LOACH);
        s32 speciesIndex = this->species - gFishmodSpecies;
        s32 role;

        OPEN_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
        for (role = 0; role < FISHMOD_TEX_ROLE_MAX; role++) {
            void* tex = sSpeciesTextures[speciesIndex][role];

            if (tex == NULL) {
                tex = sTextureRoles[isLoach][role];
                if (tex == NULL) {
                    continue;
                }
                tex = SEGMENTED_TO_VIRTUAL(tex);
            }
            gSPSegment(POLY_OPA_DISP++, 0x08 + role, tex);
        }
        CLOSE_DISPS(play->state.gfxCtx, "../z_en_fishmod.c", __LINE__);
    }

    shape = this->species->shape;

    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    Matrix_RotateY(BINANG_TO_RAD_ALT(this->actor.shape.rot.y), MTXMODE_APPLY);
    Matrix_RotateX(BINANG_TO_RAD_ALT(this->actor.shape.rot.x), MTXMODE_APPLY);
    Matrix_RotateZ(BINANG_TO_RAD_ALT(this->actor.shape.rot.z), MTXMODE_APPLY);
    Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);

    if (this->species->base == FISHMOD_BASE_LOACH) {
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

    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount,
                          EnFishmod_OverrideLimbDraw, EnFishmod_PostLimbDraw, this);

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
