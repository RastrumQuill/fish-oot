/*
 * File: fishmod_rig.c
 * Overlay: ovl_En_Fishmod
 * Description: the fish renderer used from FISHMOD_OPT 4 (the fish manager draws every fish with it).
 *
 * The pond models have a limb for every fin, most of them a single triangle that never moves on its own, and the
 * generic skeleton code builds a full matrix for each one (plus scale and unscale matrices for the per-species part
 * shapes). Here each model gets a "rig":
 *
 * - Only the limbs that move (the head, the body joints, the jaw and the pectoral fins) get a matrix each frame.
 *   Every other limb is folded into the moving limb it hangs from: its static pose and the species' part scales
 *   are baked into a per-species copy of its vertices, so it draws with its owner's matrix and looks the same.
 * - All the limb display lists of a model are copied into one display list, with the matrix loads pointed at the
 *   moving limbs (segment 0x0D), the vertices at the species' baked copy (segment 0x0E) and the part tints at the
 *   species' tint table (segment 0x0F). Drawing a fish is then a handful of matrices and one display list call.
 * - Each model has a full rig and a reduced one (head, middle and tail only), for mid-distance fish and small fish.
 *
 * The matrices are built directly (yaw/pitch/roll from the sine table, no matrix stack).
 */

#include "z_en_fishmod.h"
#include "assets/objects/object_fish/object_fish.h"
#include "assets/objects/gameplay_keep/gameplay_keep_0x18610.h"
#include "assets/objects/gameplay_keep/gFishInWaterAnim.h"
#include "animation.h"
#include "gfx.h"
#include "segmented_address.h"
#include "sys_matrix.h"
#include "z_lib.h"
#include "zelda_arena.h"
#include "play_state.h"
#include "ultra64/libc.h"
#include "buffers.h"

#if FISHMOD_OPT >= 4

#define RIG_MAX_LOADS 48
#define RIG_SEG_MTX 0x0D
#define RIG_SEG_VTX 0x0E
#define RIG_SEG_TINT 0x0F

typedef struct RigMtx {
    f32 m[3][4]; // rows; column 3 is the translation
} RigMtx;

typedef struct FishRigVtxLoad {
    Vtx* src;  // the model's vertices (virtual address)
    u16 dst;   // first vertex in the baked copy
    u8 n;
    u8 limb;   // the limb whose matrix is current when they're loaded
} FishRigVtxLoad;

/** A model's rig (one per model per variant): which limbs move, and the shared display list. */
typedef struct FishRigBase {
    u8 ready;
    u8 limbCount;
    u8 dlCount;
    u8 slotCount;
    u8 parent[EN_FISHMOD_MAX_LIMBS];   // 0xFF for the root
    u8 slotOf[EN_FISHMOD_MAX_LIMBS];   // moving limb -> its slot, 0xFF if folded
    u8 owner[EN_FISHMOD_MAX_LIMBS];    // the slot every limb draws with
    u8 dlLimb[EN_FISHMOD_MAX_LIMBS];   // display list order (as the skeleton code draws) -> limb
    u8 slotLimb[FISH_RIG_MAX_SLOTS];
    u8 slotParent[FISH_RIG_MAX_SLOTS]; // the slot of the moving limb its parent folds into, 0xFF for the root
    u8 slotAnim[FISH_RIG_MAX_SLOTS];   // FishRigAnim: which swim angles turn it
    Vec3s pose[EN_FISHMOD_MAX_LIMBS + 1]; // the model's rest pose (root translation, then limb rotations)
    Vec3s jointPos[EN_FISHMOD_MAX_LIMBS];
    u16 vtxCount;
    u8 loadCount;
    FishRigVtxLoad loads[RIG_MAX_LOADS];
    Gfx* dl;
} FishRigBase;

/** A species' copy of a model's rig: its baked vertices, tints and the static part of each moving limb's matrix. */
typedef struct FishRigSpecies {
    Vtx* vtx;
    Gfx tints[FISHMOD_PART_MAX][2];
    RigMtx pre[FISH_RIG_MAX_SLOTS]; // parent slot frame -> this limb's joint, before its rotation
    Vec3f origin[EN_FISHMOD_MAX_LIMBS]; // every limb's joint in its owner's frame (for the bounds)
} FishRigSpecies;

typedef enum FishRigAnim {
    RIG_ANIM_NONE,
    RIG_ANIM_BODY1,    // y += bodyYaw[1]
    RIG_ANIM_BODY2,    // y += bodyYaw[2]
    RIG_ANIM_BODY3,    // y += bodyYaw[3]
    RIG_ANIM_BODY12,   // y += bodyYaw[1] + bodyYaw[2] (reduced rig: the two front joints in one)
    RIG_ANIM_FIN_POS,  // y += finYaw
    RIG_ANIM_FIN_NEG,  // y -= finYaw
    RIG_ANIM_JAW       // z -= jawRot
} FishRigAnim;

typedef struct FishRigModelDef {
    u8 movers[2][FISH_RIG_MAX_SLOTS]; // per variant: limb index + 1 (0 ends the list), parents first
    u8 anims[2][FISH_RIG_MAX_SLOTS];
} FishRigModelDef;

// The limbs that move and how, matching the old limb callbacks (EnFishmod_OverrideLimbDraw).
static FishRigModelDef sRigModels[FISH_RIG_MODELS] = {
    // bass: head, three body joints, jaw, pectoral fins
    { { { 1, 2, 3, 4, 13, 14, 15, 0 }, { 1, 2, 4, 0 } },
      { { RIG_ANIM_NONE, RIG_ANIM_BODY1, RIG_ANIM_BODY2, RIG_ANIM_BODY3, RIG_ANIM_JAW, RIG_ANIM_FIN_NEG,
          RIG_ANIM_FIN_POS },
        { RIG_ANIM_NONE, RIG_ANIM_BODY12, RIG_ANIM_BODY3 } } },
    // loach: head, three body joints, pectoral fins
    { { { 1, 3, 4, 5, 9, 10, 0 }, { 1, 3, 5, 0 } },
      { { RIG_ANIM_NONE, RIG_ANIM_BODY1, RIG_ANIM_BODY2, RIG_ANIM_BODY3, RIG_ANIM_FIN_POS, RIG_ANIM_FIN_NEG },
        { RIG_ANIM_NONE, RIG_ANIM_BODY12, RIG_ANIM_BODY3 } } },
    // small fish: head, rear body, tail (it only ever had these)
    { { { 1, 4, 6, 0 }, { 1, 4, 6, 0 } },
      { { RIG_ANIM_NONE, RIG_ANIM_BODY1, RIG_ANIM_BODY3 }, { RIG_ANIM_NONE, RIG_ANIM_BODY1, RIG_ANIM_BODY3 } } },
};

// not in the headers (z_skelanime.c)
void SkelAnime_GetFrameData(AnimationHeader* animation, s32 frame, s32 limbCount, Vec3s* frameTable);

static FishRigBase sRigBases[FISH_RIG_MODELS][FISH_RIG_VARIANTS];
static FishRigSpecies* sRigSpecies[FISHMOD_SPECIES_MAX][FISH_RIG_VARIANTS];

/* ---------------------------------------------------------------------------------------------------------------
 * Matrix helpers (affine, rows of the usual column-vector convention: v' = M v)
 */

static void Rig_Identity(RigMtx* m) {
    bzero(m, sizeof(RigMtx));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1.0f;
}

/** out = a * b */
static void Rig_Mul(RigMtx* out, RigMtx* a, RigMtx* b) {
    RigMtx r;
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 a0 = a->m[i][0];
        f32 a1 = a->m[i][1];
        f32 a2 = a->m[i][2];

        r.m[i][0] = a0 * b->m[0][0] + a1 * b->m[1][0] + a2 * b->m[2][0];
        r.m[i][1] = a0 * b->m[0][1] + a1 * b->m[1][1] + a2 * b->m[2][1];
        r.m[i][2] = a0 * b->m[0][2] + a1 * b->m[1][2] + a2 * b->m[2][2];
        r.m[i][3] = a0 * b->m[0][3] + a1 * b->m[1][3] + a2 * b->m[2][3] + a->m[i][3];
    }
    *out = r;
}

/** m = m * T(x, y, z) */
static void Rig_Translate(RigMtx* m, f32 x, f32 y, f32 z) {
    s32 i;

    for (i = 0; i < 3; i++) {
        m->m[i][3] += m->m[i][0] * x + m->m[i][1] * y + m->m[i][2] * z;
    }
}

/** m = m * S(x, y, z) */
static void Rig_Scale(RigMtx* m, f32 x, f32 y, f32 z) {
    s32 i;

    for (i = 0; i < 3; i++) {
        m->m[i][0] *= x;
        m->m[i][1] *= y;
        m->m[i][2] *= z;
    }
}

/** m = m * Ry(y) */
static void Rig_RotateY(RigMtx* m, s16 y) {
    f32 s = Math_SinS(y);
    f32 c = Math_CosS(y);
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 cx = m->m[i][0];
        f32 cz = m->m[i][2];

        m->m[i][0] = cx * c - cz * s;
        m->m[i][2] = cx * s + cz * c;
    }
}

/** m = m * Rx(x) */
static void Rig_RotateX(RigMtx* m, s16 x) {
    f32 s = Math_SinS(x);
    f32 c = Math_CosS(x);
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 cy = m->m[i][1];
        f32 cz = m->m[i][2];

        m->m[i][1] = cy * c + cz * s;
        m->m[i][2] = cz * c - cy * s;
    }
}

/** m = m * Rz(z) */
static void Rig_RotateZ(RigMtx* m, s16 z) {
    f32 s = Math_SinS(z);
    f32 c = Math_CosS(z);
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 cx = m->m[i][0];
        f32 cy = m->m[i][1];

        m->m[i][0] = cx * c + cy * s;
        m->m[i][1] = cy * c - cx * s;
    }
}

/** m = m * Rz * Ry * Rx, the order the skeleton code uses (Matrix_TranslateRotateZYX) */
static void Rig_RotateZYX(RigMtx* m, s16 x, s16 y, s16 z) {
    if (z != 0) {
        Rig_RotateZ(m, z);
    }
    if (y != 0) {
        Rig_RotateY(m, y);
    }
    if (x != 0) {
        Rig_RotateX(m, x);
    }
}

static void Rig_ToMtx(RigMtx* m, Mtx* out) {
    MtxF f;

    f.xx = m->m[0][0];
    f.xy = m->m[0][1];
    f.xz = m->m[0][2];
    f.xw = m->m[0][3];
    f.yx = m->m[1][0];
    f.yy = m->m[1][1];
    f.yz = m->m[1][2];
    f.yw = m->m[1][3];
    f.zx = m->m[2][0];
    f.zy = m->m[2][1];
    f.zz = m->m[2][2];
    f.zw = m->m[2][3];
    f.wx = f.wy = f.wz = 0.0f;
    f.ww = 1.0f;
    Matrix_MtxFToMtx(&f, out);
}

static void Rig_MulPoint(RigMtx* m, Vec3f* in, Vec3f* out) {
    out->x = m->m[0][0] * in->x + m->m[0][1] * in->y + m->m[0][2] * in->z + m->m[0][3];
    out->y = m->m[1][0] * in->x + m->m[1][1] * in->y + m->m[1][2] * in->z + m->m[1][3];
    out->z = m->m[2][0] * in->x + m->m[2][1] * in->y + m->m[2][2] * in->z + m->m[2][3];
}

/* ---------------------------------------------------------------------------------------------------------------
 * Building
 */

static FlexSkeletonHeader* Rig_Skeleton(s32 model) {
    return (model == FISH_RIG_LOACH) ? &gFishingLoachSkel : (model == FISH_RIG_SMALL) ? &gFishSkel : &gFishingFishSkel;
}

static AnimationHeader* Rig_Anim(s32 model) {
    return (model == FISH_RIG_LOACH) ? &gFishingLoachAnim : (model == FISH_RIG_SMALL) ? &gFishInWaterAnim
                                                                                      : &gFishingFishAnim;
}

s32 EnFishmod_RigModel(const FishmodSpecies* species) {
    if (species->base == FISHMOD_BASE_LOACH) {
        return FISH_RIG_LOACH;
    }
    if (species->base == FISHMOD_BASE_SMALL) {
        return FISH_RIG_SMALL;
    }
    return FISH_RIG_BASS;
}

/** The display list order: limbs with a display list, depth first, children before siblings (as the skeleton code). */
static void Rig_ListDLs(FishRigBase* base, void** limbs, s32 limbIndex) {
    StandardLimb* limb = SEGMENTED_TO_VIRTUAL(limbs[limbIndex]);

    if (limb->dList != NULL) {
        base->dlLimb[base->dlCount++] = limbIndex;
    }
    if (limb->child != LIMB_DONE) {
        Rig_ListDLs(base, limbs, limb->child);
    }
    if (limb->sibling != LIMB_DONE) {
        Rig_ListDLs(base, limbs, limb->sibling);
    }
}

/** A limb's display list, as the fish draw it (the small fish use the overlay's patched copies). */
static Gfx* Rig_LimbDL(s32 model, StandardLimb* limb) {
    return (model == FISH_RIG_SMALL) ? EnFishmod_SmallDL(limb->dList) : SEGMENTED_TO_VIRTUAL(limb->dList);
}

static s32 Rig_BuildBase(s32 model, s32 variant, u8* partMap) {
    FishRigBase* base = &sRigBases[model][variant];
    FlexSkeletonHeader* skel = SEGMENTED_TO_VIRTUAL(Rig_Skeleton(model));
    void** limbs = SEGMENTED_TO_VIRTUAL(skel->sh.segment);
    FishRigModelDef* def = &sRigModels[model];
    s32 gfxCount;
    s32 i;
    s32 k;
    Gfx* out;
    u8 lastPart = 0xFE;

    bzero(base, sizeof(*base));
    base->limbCount = CLAMP_MAX(skel->sh.limbCount, EN_FISHMOD_MAX_LIMBS);

    for (i = 0; i < base->limbCount; i++) {
        base->parent[i] = 0xFF;
        base->slotOf[i] = 0xFF;
    }
    for (i = 0; i < base->limbCount; i++) {
        StandardLimb* limb = SEGMENTED_TO_VIRTUAL(limbs[i]);
        s32 child = limb->child;

        base->jointPos[i] = limb->jointPos;
        while ((child != LIMB_DONE) && (child < base->limbCount)) {
            base->parent[child] = i;
            child = ((StandardLimb*)SEGMENTED_TO_VIRTUAL(limbs[child]))->sibling;
        }
    }
    SkelAnime_GetFrameData(Rig_Anim(model), 0, base->limbCount + 1, base->pose);

    for (k = 0; (k < FISH_RIG_MAX_SLOTS) && (def->movers[variant][k] != 0); k++) {
        s32 limb = def->movers[variant][k] - 1;

        base->slotLimb[k] = limb;
        base->slotAnim[k] = def->anims[variant][k];
        base->slotOf[limb] = k;
    }
    base->slotCount = k;

    // every limb draws with the nearest moving limb at or above it
    for (i = 0; i < base->limbCount; i++) {
        s32 l = i;

        while ((l != 0xFF) && (base->slotOf[l] == 0xFF)) {
            l = base->parent[l];
        }
        base->owner[i] = (l == 0xFF) ? 0 : base->slotOf[l];
    }
    for (k = 0; k < base->slotCount; k++) {
        s32 p = base->parent[base->slotLimb[k]];

        base->slotParent[k] = (p == 0xFF) ? 0xFF : base->owner[p];
    }

    Rig_ListDLs(base, limbs, 0);

    // the shared display list: per limb a tint, its owner's matrix and a copy of its display list
    gfxCount = 1;
    for (i = 0; i < base->dlCount; i++) {
        Gfx* g = Rig_LimbDL(model, SEGMENTED_TO_VIRTUAL(limbs[base->dlLimb[i]]));

        while ((g->words.w0 >> 24) != G_ENDDL) {
            g++;
            gfxCount++;
        }
        gfxCount += 2;
    }
    base->dl = ZeldaArena_Malloc(gfxCount * sizeof(Gfx));
    if (base->dl == NULL) {
        return false;
    }
    out = base->dl;

    for (i = 0; i < base->dlCount; i++) {
        s32 limbIndex = base->dlLimb[i];
        s32 matLimb = limbIndex;
        Gfx* g = Rig_LimbDL(model, SEGMENTED_TO_VIRTUAL(limbs[limbIndex]));
        u8 part = partMap[limbIndex];

        if ((part != FISHMOD_PART_NONE) && (part != lastPart)) {
            gSPDisplayList(out++, SEGMENT_ADDR(RIG_SEG_TINT, part * 2 * sizeof(Gfx)));
            lastPart = part;
        }
        gSPMatrix(out++, SEGMENT_ADDR(RIG_SEG_MTX, base->owner[limbIndex] * sizeof(Mtx)),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        for (; (g->words.w0 >> 24) != G_ENDDL; g++) {
            u8 op = g->words.w0 >> 24;

            *out = *g;
            if ((op == G_MTX) && ((g->words.w1 >> 24) == RIG_SEG_MTX)) {
                // a seam: these vertices go with another limb's matrix
                s32 k2 = (g->words.w1 & 0xFFFFFF) / sizeof(Mtx);

                matLimb = (k2 < base->dlCount) ? base->dlLimb[k2] : limbIndex;
                out->words.w1 = SEGMENT_ADDR(RIG_SEG_MTX, base->owner[matLimb] * sizeof(Mtx));
            } else if (op == G_VTX) {
                s32 n = (g->words.w0 >> 12) & 0xFF;
                FishRigVtxLoad* load;

                if (base->loadCount >= RIG_MAX_LOADS) {
                    return false;
                }
                load = &base->loads[base->loadCount++];
                load->src = SEGMENTED_TO_VIRTUAL(g->words.w1);
                load->n = n;
                load->dst = base->vtxCount;
                load->limb = matLimb;
                out->words.w1 = SEGMENT_ADDR(RIG_SEG_VTX, base->vtxCount * sizeof(Vtx));
                base->vtxCount += n;
            }
            out++;
        }
    }
    gSPEndDisplayList(out++);
    base->ready = true;
    return true;
}

/** The static transform from a limb's owner's frame to the limb's own (unscaled) frame. */
static void Rig_StaticFrame(FishRigBase* base, Vec3f* scales, s32 limb, RigMtx* out) {
    s32 p;

    if (base->slotOf[limb] != 0xFF) {
        Rig_Identity(out);
        return;
    }
    p = base->parent[limb];
    if (p == 0xFF) {
        Rig_Identity(out);
    } else {
        Rig_StaticFrame(base, scales, p, out);
    }
    Rig_Translate(out, base->jointPos[limb].x * ((p == 0xFF) ? 1.0f : scales[p].x),
                  base->jointPos[limb].y * ((p == 0xFF) ? 1.0f : scales[p].y),
                  base->jointPos[limb].z * ((p == 0xFF) ? 1.0f : scales[p].z));
    Rig_RotateZYX(out, base->pose[limb + 1].x, base->pose[limb + 1].y, base->pose[limb + 1].z);
}

static FishRigSpecies* Rig_BuildSpecies(const FishmodSpecies* species, s32 model, s32 variant, u8* partMap) {
    FishRigBase* base = &sRigBases[model][variant];
    FishRigSpecies* rig;
    Vec3f scales[EN_FISHMOD_MAX_LIMBS];
    s32 i;
    s32 k;

    if (!base->ready && !Rig_BuildBase(model, variant, partMap)) {
        return NULL;
    }
    rig = ZeldaArena_Malloc(sizeof(FishRigSpecies));
    if (rig == NULL) {
        return NULL;
    }
    rig->vtx = ZeldaArena_Malloc(base->vtxCount * sizeof(Vtx));
    if (rig->vtx == NULL) {
        ZeldaArena_Free(rig);
        return NULL;
    }

    for (i = 0; i < base->limbCount; i++) {
        u8 part = partMap[i];

        if (part == FISHMOD_PART_NONE) {
            scales[i].x = scales[i].y = scales[i].z = 1.0f;
        } else {
            scales[i].x = species->partScale[part][0] * 0.01f;
            scales[i].y = species->partScale[part][1] * 0.01f;
            scales[i].z = species->partScale[part][2] * 0.01f;
        }
    }

    for (i = 0; i < FISHMOD_PART_MAX; i++) {
        const u8* tint = species->partTint[i];

        gDPSetPrimColor(&rig->tints[i][0], 0, 0x80, tint[0], tint[1], tint[2], 255);
        gSPEndDisplayList(&rig->tints[i][1]);
    }

    // each moving limb: its parent's static frame (in the parent slot's frame), then its joint
    for (k = 0; k < base->slotCount; k++) {
        s32 limb = base->slotLimb[k];
        s32 p = base->parent[limb];

        if (p == 0xFF) {
            Rig_Identity(&rig->pre[k]);
            Rig_Translate(&rig->pre[k], base->pose[0].x, base->pose[0].y, base->pose[0].z);
        } else {
            Rig_StaticFrame(base, scales, p, &rig->pre[k]);
            Rig_Translate(&rig->pre[k], base->jointPos[limb].x * scales[p].x, base->jointPos[limb].y * scales[p].y,
                          base->jointPos[limb].z * scales[p].z);
        }
    }

    for (i = 0; i < base->limbCount; i++) {
        RigMtx f;

        Rig_StaticFrame(base, scales, i, &f);
        rig->origin[i].x = f.m[0][3];
        rig->origin[i].y = f.m[1][3];
        rig->origin[i].z = f.m[2][3];
    }

    // bake the vertices: static pose and part scale of the limb they belong to, into its owner's frame
    for (i = 0; i < base->loadCount; i++) {
        FishRigVtxLoad* load = &base->loads[i];
        RigMtx f;
        s32 v;

        Rig_StaticFrame(base, scales, load->limb, &f);
        Rig_Scale(&f, scales[load->limb].x, scales[load->limb].y, scales[load->limb].z);
        for (v = 0; v < load->n; v++) {
            Vtx* src = &load->src[v];
            Vtx* dst = &rig->vtx[load->dst + v];
            Vec3f p;
            Vec3f q;
            s32 c;

            *dst = *src;
            p.x = src->n.ob[0];
            p.y = src->n.ob[1];
            p.z = src->n.ob[2];
            Rig_MulPoint(&f, &p, &q);
            dst->n.ob[0] = (s16)CLAMP(q.x, -0x7FFF, 0x7FFF);
            dst->n.ob[1] = (s16)CLAMP(q.y, -0x7FFF, 0x7FFF);
            dst->n.ob[2] = (s16)CLAMP(q.z, -0x7FFF, 0x7FFF);
            // normals go through the same linear part (lighting saw them through the scaled matrix before)
            for (c = 0; c < 3; c++) {
                f32 n = f.m[c][0] * src->n.n[0] + f.m[c][1] * src->n.n[1] + f.m[c][2] * src->n.n[2];

                dst->n.n[c] = (s8)CLAMP(n, -127.0f, 127.0f);
            }
        }
    }
    return rig;
}

/** Makes sure this fish's species has its rigs (built the first time a species is seen). */
s32 EnFishmod_RigPrepare(EnFishmod* this) {
    s32 speciesIndex = this->species - gFishmodSpecies;
    s32 model = EnFishmod_RigModel(this->species);
    s32 variant;

    for (variant = 0; variant < FISH_RIG_VARIANTS; variant++) {
        if (sRigSpecies[speciesIndex][variant] == NULL) {
            sRigSpecies[speciesIndex][variant] =
                Rig_BuildSpecies(this->species, model, variant, EnFishmod_PartMap(this->species));
            if (sRigSpecies[speciesIndex][variant] == NULL) {
                return false;
            }
        }
    }
    return true;
}

/** Frees every rig (the fish manager does this when it goes, after the last frame that drew with them). */
void EnFishmod_RigFreeAll(void) {
    s32 i;
    s32 v;

    for (i = 0; i < FISHMOD_SPECIES_MAX; i++) {
        for (v = 0; v < FISH_RIG_VARIANTS; v++) {
            if (sRigSpecies[i][v] != NULL) {
                ZeldaArena_Free(sRigSpecies[i][v]->vtx);
                ZeldaArena_Free(sRigSpecies[i][v]);
                sRigSpecies[i][v] = NULL;
            }
        }
    }
    for (i = 0; i < FISH_RIG_MODELS; i++) {
        for (v = 0; v < FISH_RIG_VARIANTS; v++) {
            if (sRigBases[i][v].dl != NULL) {
                ZeldaArena_Free(sRigBases[i][v].dl);
            }
            bzero(&sRigBases[i][v], sizeof(FishRigBase));
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Drawing
 */

/** Points segments 0x0E/0x0F at a species' baked vertices and tints (once per species and variant). */
void EnFishmod_RigSetSpecies(EnFishmod* this, PlayState* play, s32 variant) {
    FishRigSpecies* rig = sRigSpecies[this->species - gFishmodSpecies][variant];

    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rig.c", __LINE__);
    gSPSegment(POLY_OPA_DISP++, RIG_SEG_VTX, rig->vtx);
    gSPSegment(POLY_OPA_DISP++, RIG_SEG_TINT, rig->tints);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rig.c", __LINE__);
}

/** Gives segments 0x0E/0x0F back (the z-buffer and the framebuffer) after the fish are drawn. */
void EnFishmod_RigRestoreSegments(PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx, "../fishmod_rig.c", __LINE__);
    gSPSegment(POLY_OPA_DISP++, 0x0E, gZBuffer);
    gSPSegment(POLY_OPA_DISP++, 0x0F, play->state.gfxCtx->curFrameBuffer);
    CLOSE_DISPS(play->state.gfxCtx, "../fishmod_rig.c", __LINE__);
}

/**
 * Draws a fish with its rig (EnFishmod_RigSetSpecies first). Also updates its head position, and its bounds when
 * something needs them (held up by Link, photos, whiskers).
 */
void EnFishmod_RigDraw(EnFishmod* this, PlayState* play, s32 variant, s32 wantBounds) {
    s32 model = EnFishmod_RigModel(this->species);
    FishRigBase* base = &sRigBases[model][variant];
    FishRigSpecies* rig = sRigSpecies[this->species - gFishmodSpecies][variant];
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Mtx* mtx = GRAPH_ALLOC(gfxCtx, base->slotCount * sizeof(Mtx));
    RigMtx frames[FISH_RIG_MAX_SLOTS];
    RigMtx root;
    const u8* shape = this->species->shape;
    f32 amp = (variant == FISH_RIG_REDUCED) && (this->lengthInches < FISHMOD_SMALL_RIG_LENGTH) ? 1.3f : 1.0f;
    s32 k;
#if FISHMOD_DEBUG
    OSTime profStart = osGetTime();
#endif

    // the fish's root, as EnFishmod_Draw sets it up for the skeleton
    Rig_Identity(&root);
    root.m[0][3] = this->actor.world.pos.x;
    root.m[1][3] = this->actor.world.pos.y;
    root.m[2][3] = this->actor.world.pos.z;
    Rig_RotateY(&root, this->actor.shape.rot.y);
    Rig_RotateX(&root, this->actor.shape.rot.x);
    Rig_RotateZ(&root, this->actor.shape.rot.z);
    Rig_Scale(&root, this->actor.scale.x, this->actor.scale.y, this->actor.scale.z);
    if (model == FISH_RIG_SMALL) {
        Rig_RotateY(&root, (s16)(this->bodyYaw[0] / 2));
        Rig_Scale(&root, shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f);
        Rig_Translate(&root, -650.0f, 0.0f, 0.0f);
    } else if (model == FISH_RIG_LOACH) {
        Rig_RotateY(&root, (s16)(this->bodyYaw[0] / 2 - 0x4000));
        Rig_Scale(&root, shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f);
        Rig_Translate(&root, -8000.0f, 500.0f, 0.0f);
    } else {
        Rig_RotateY(&root, (s16)(this->bodyYaw[0] / 2 - 0x4000));
        Rig_Scale(&root, shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f);
        Rig_Translate(&root, -2300.0f, -300.0f, 0.0f);
    }
#if FISHMOD_DEBUG
    gFishmodProf[FISHMOD_PROF_D_ROOT] += osGetTime() - profStart;
    gFishmodProfCount[FISHMOD_PROF_D_ROOT]++;
    profStart = osGetTime();
#endif

    for (k = 0; k < base->slotCount; k++) {
        s32 limb = base->slotLimb[k];
        Vec3s* rot = &base->pose[limb + 1];
        s16 dy = 0;
        s16 dz = 0;

        switch (base->slotAnim[k]) {
            case RIG_ANIM_BODY1:
                dy = this->bodyYaw[1] * amp;
                break;
            case RIG_ANIM_BODY2:
                dy = this->bodyYaw[2];
                break;
            case RIG_ANIM_BODY3:
                dy = this->bodyYaw[3] * amp;
                break;
            case RIG_ANIM_BODY12:
                dy = (this->bodyYaw[1] + this->bodyYaw[2]) * amp;
                break;
            case RIG_ANIM_FIN_POS:
                dy = this->finYaw;
                break;
            case RIG_ANIM_FIN_NEG:
                dy = -this->finYaw;
                break;
            case RIG_ANIM_JAW:
                dz = -this->jawRot;
                break;
            default:
                break;
        }
        Rig_Mul(&frames[k], (base->slotParent[k] == 0xFF) ? &root : &frames[base->slotParent[k]], &rig->pre[k]);
        Rig_RotateZYX(&frames[k], rot->x, rot->y + dy, rot->z + dz);
        Rig_ToMtx(&frames[k], &mtx[k]);
    }

    this->headPos.x = frames[0].m[0][3];
    this->headPos.y = frames[0].m[1][3];
    this->headPos.z = frames[0].m[2][3];
    if (wantBounds) {
        s32 i;

        this->boundsMin = this->boundsMax = this->headPos;
        for (i = 1; i < base->limbCount; i++) {
            Vec3f p;

            Rig_MulPoint(&frames[base->owner[i]], &rig->origin[i], &p);
            this->boundsMin.x = CLAMP_MAX(this->boundsMin.x, p.x);
            this->boundsMin.y = CLAMP_MAX(this->boundsMin.y, p.y);
            this->boundsMin.z = CLAMP_MAX(this->boundsMin.z, p.z);
            this->boundsMax.x = CLAMP_MIN(this->boundsMax.x, p.x);
            this->boundsMax.y = CLAMP_MIN(this->boundsMax.y, p.y);
            this->boundsMax.z = CLAMP_MIN(this->boundsMax.z, p.z);
        }
    }

    OPEN_DISPS(gfxCtx, "../fishmod_rig.c", __LINE__);
    gSPSegment(POLY_OPA_DISP++, RIG_SEG_MTX, mtx);
    gSPDisplayList(POLY_OPA_DISP++, base->dl);
    CLOSE_DISPS(gfxCtx, "../fishmod_rig.c", __LINE__);
#if FISHMOD_DEBUG
    gFishmodProf[FISHMOD_PROF_D_POSE] += osGetTime() - profStart;
    gFishmodProfCount[FISHMOD_PROF_D_POSE]++;
#endif
}

#endif
