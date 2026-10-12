/*
 * File: fishmod_rig.c
 * Overlay: ovl_En_Fishmod
 * Description: the fish renderer used from FISHMOD_OPT 4 (the fish manager draws every fish with it).
 *
 * The pond models only bend at three joints, the first right behind the gills, so the whole body swings from the
 * "neck"; and they have a limb for every fin, most of them a single triangle that never moves on its own. Here each
 * model gets a "rig" built from its rest pose instead:
 *
 * - A spine of bones along the body: the head (everything in front of a pivot about 0.3 body lengths behind the
 *   snout, where real fish turn about) and a run of body bones behind it, so each ring of vertices along the body can
 *   bend by itself. Every vertex goes with the bone that covers its place along the body; the species' part shapes
 *   and each limb's rest pose are baked into a per-species copy of the vertices, relative to their bone.
 * - The fins that move on their own (pectorals, the bass's jaw) keep their own small bones.
 * - The swimming is the body wave of fish biomechanics, y(x, t) = A(x) sin(kx - wt) (Lighthill; Videler & Hess
 *   1984): almost no sideways movement at the front, rising steeply to the tail, a little head recoil the other way.
 *   Each bone turns by the wave's slope at its place. Turns and startles curve the whole body.
 * - All the limb display lists of a model are copied into one display list, with matrix loads per bone
 *   (segment 0x0D), the vertices at the species' baked copy (segment 0x0E) and the part tints at the species' tint
 *   table (segment 0x0F). Drawing a fish is a handful of matrices and one display list call.
 * - Each model has a full rig and a reduced one (fewer body bones, fins still), for mid-distance and small fish.
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
#define RIG_MAX_FINS 3
#define RIG_MAX_JOINT 0x1300 // the sharpest bend between two body bones (about 27 degrees)
#define RIG_SEG_MTX 0x0D
#define RIG_SEG_VTX 0x0E
#define RIG_SEG_TINT 0x0F
#define RIG_PIVOT 0.3f // where the head bone ends and the fish turns about, as a fraction of the body from the snout

typedef struct RigMtx {
    f32 m[3][4]; // rows; column 3 is the translation
} RigMtx;

typedef struct FishRigVtxLoad {
    Vtx* src;  // the model's vertices (virtual address)
    u16 dst;   // first vertex in the baked copy
    u8 n;
    u8 limb;   // the limb whose matrix the model loads them with
} FishRigVtxLoad;

typedef enum FishRigAnim {
    RIG_ANIM_FIN_POS, // y += finYaw
    RIG_ANIM_FIN_NEG, // y -= finYaw
    RIG_ANIM_JAW      // z -= jawRot
} FishRigAnim;

/** A model's rig (one per model per variant): the bones, which bone every vertex goes with, the display list. */
typedef struct FishRigBase {
    u8 ready;
    u8 limbCount;
    u8 dlCount;
    u8 bodyBones; // spine bones behind the pivot (bone 0 is the head)
    u8 finCount;  // fin bones, after the body bones
    u8 boneCount;
    u8 parent[EN_FISHMOD_MAX_LIMBS];  // 0xFF for the root
    u8 kept[EN_FISHMOD_MAX_LIMBS];    // the head and the fin limbs: frames that vertices are baked in
    u8 finOf[EN_FISHMOD_MAX_LIMBS];   // 1 + the fin a limb belongs to (itself or an ancestor), 0 if body
    u8 dlLimb[EN_FISHMOD_MAX_LIMBS];  // display list order (as the skeleton code draws) -> limb
    u8 finLimb[RIG_MAX_FINS];
    u8 finAnim[RIG_MAX_FINS];
    u8 tailLimb;  // the joint the spine runs toward
    u8 bendLimb;  // the first body joint: its up axis is the axis the body bends about
    Vec3s pose[EN_FISHMOD_MAX_LIMBS + 1]; // the model's rest pose (root translation, then limb rotations)
    Vec3s jointPos[EN_FISHMOD_MAX_LIMBS];
    u16 vtxCount;
    u8 loadCount;
    FishRigVtxLoad loads[RIG_MAX_LOADS];
    u8* vtxBone; // per baked vertex
    Gfx* dl;
} FishRigBase;

/** A species' copy of a model's rig: its baked vertices and tints, and the bones' rest places (shapes differ). */
typedef struct FishRigSpecies {
    Vtx* vtx;
    Gfx tints[FISHMOD_PART_MAX][2];
    RigMtx headFrame;                  // the head limb's rest frame in model space
    Vec3f axis;                        // along the spine toward the tail (head frame)
    Vec3f up;                          // what the body bends about (head frame)
    Vec3f bonePos[FISH_RIG_MAX_SLOTS]; // rest origins in the head frame (the head bone's is the pivot)
    RigMtx finPre[RIG_MAX_FINS];       // fin bone: its parent bone's frame -> the fin's joint, before its rotation
    u8 finParent[RIG_MAX_FINS];
    Vec3f headPoint; // the head limb's origin, relative to the head bone (for the head position)
    Vec3f tailPoint; // the tail tip, relative to the last body bone
} FishRigSpecies;

/** How each model swims: body wave amplitude A(x) = a0 + a1 x + a2 x^2 (body lengths) and wavelength. */
typedef struct FishRigSwim {
    f32 a0;
    f32 a1;
    f32 a2;
    f32 wavelength;
} FishRigSwim;

typedef struct FishRigModelDef {
    u8 bodyBones[FISH_RIG_VARIANTS];
    u8 tailLimb;
    u8 bendLimb;
    u8 finLimbs[RIG_MAX_FINS]; // limb index + 1, 0 = none (full rig only)
    u8 finAnims[RIG_MAX_FINS];
    FishRigSwim swim;
} FishRigModelDef;

static FishRigModelDef sRigModels[FISH_RIG_MODELS] = {
    // bass: carangiform (most of the motion in the back half); jaw and pectoral fins
    { { 5, 2 }, 3, 1, { 13, 14, 15 }, { RIG_ANIM_JAW, RIG_ANIM_FIN_NEG, RIG_ANIM_FIN_POS },
      { 0.02f, -0.08f, 0.16f, 1.0f } },
    // loach: eel-like (anguilliform), the whole body waves with a shorter wave; pectoral fins
    { { 7, 3 }, 4, 2, { 9, 10, 0 }, { RIG_ANIM_FIN_POS, RIG_ANIM_FIN_NEG, 0 }, { 0.04f, -0.02f, 0.07f, 0.65f } },
    // small fish: carangiform
    { { 3, 2 }, 5, 3, { 0, 0, 0 }, { 0, 0, 0 }, { 0.02f, -0.08f, 0.16f, 1.0f } },
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

/** m = m * R(axis u, angle): u must be a unit vector */
static void Rig_RotateAxis(RigMtx* m, Vec3f* u, s16 angle) {
    f32 s = Math_SinS(angle);
    f32 c = Math_CosS(angle);
    f32 t = 1.0f - c;
    f32 r[3][3];
    s32 i;

    r[0][0] = c + u->x * u->x * t;
    r[0][1] = u->x * u->y * t - u->z * s;
    r[0][2] = u->x * u->z * t + u->y * s;
    r[1][0] = u->y * u->x * t + u->z * s;
    r[1][1] = c + u->y * u->y * t;
    r[1][2] = u->y * u->z * t - u->x * s;
    r[2][0] = u->z * u->x * t - u->y * s;
    r[2][1] = u->z * u->y * t + u->x * s;
    r[2][2] = c + u->z * u->z * t;
    for (i = 0; i < 3; i++) {
        f32 a0 = m->m[i][0];
        f32 a1 = m->m[i][1];
        f32 a2 = m->m[i][2];

        m->m[i][0] = a0 * r[0][0] + a1 * r[1][0] + a2 * r[2][0];
        m->m[i][1] = a0 * r[0][1] + a1 * r[1][1] + a2 * r[2][1];
        m->m[i][2] = a0 * r[0][2] + a1 * r[1][2] + a2 * r[2][2];
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

static void Rig_Normalize(Vec3f* v) {
    f32 len = sqrtf(SQ(v->x) + SQ(v->y) + SQ(v->z));

    if (len > 0.0001f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
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

/**
 * The static transform from the kept limb a limb belongs to (the head, or its fin) to the limb's own unscaled
 * frame, in its rest pose with the given part scales.
 */
static void Rig_StaticFrame(FishRigBase* base, Vec3f* scales, s32 limb, RigMtx* out) {
    s32 p;

    if (base->kept[limb]) {
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

/** The spine of a model with the given part scales: its axis, bend axis, and how far its body reaches. */
static void Rig_Spine(FishRigBase* base, Vec3f* scales, Vec3f* axis, Vec3f* up, f32* sMin, f32* sMax) {
    RigMtx f;
    s32 i;
    s32 v;
    f32 d;

    Rig_StaticFrame(base, scales, base->tailLimb, &f);
    axis->x = f.m[0][3];
    axis->y = f.m[1][3];
    axis->z = f.m[2][3];
    Rig_Normalize(axis);
    Rig_StaticFrame(base, scales, base->bendLimb, &f);
    up->x = f.m[0][1];
    up->y = f.m[1][1];
    up->z = f.m[2][1];
    // square to the spine
    d = up->x * axis->x + up->y * axis->y + up->z * axis->z;
    up->x -= axis->x * d;
    up->y -= axis->y * d;
    up->z -= axis->z * d;
    Rig_Normalize(up);

    *sMin = 1e9f;
    *sMax = -1e9f;
    for (i = 0; i < base->loadCount; i++) {
        FishRigVtxLoad* load = &base->loads[i];

        if (base->finOf[load->limb] != 0) {
            continue;
        }
        Rig_StaticFrame(base, scales, load->limb, &f);
        Rig_Scale(&f, scales[load->limb].x, scales[load->limb].y, scales[load->limb].z);
        for (v = 0; v < load->n; v++) {
            Vec3f p;
            Vec3f q;
            f32 s;

            p.x = load->src[v].n.ob[0];
            p.y = load->src[v].n.ob[1];
            p.z = load->src[v].n.ob[2];
            Rig_MulPoint(&f, &p, &q);
            s = q.x * axis->x + q.y * axis->y + q.z * axis->z;
            *sMin = CLAMP_MAX(*sMin, s);
            *sMax = CLAMP_MIN(*sMax, s);
        }
    }
}

/** Which body bone covers a place along the body (0 = head; fraction of the body from the snout). */
static s32 Rig_BoneAt(FishRigBase* base, f32 frac) {
    s32 b;

    if (frac < RIG_PIVOT) {
        return 0;
    }
    b = 1 + (s32)((frac - RIG_PIVOT) / (1.0f - RIG_PIVOT) * base->bodyBones);
    return CLAMP(b, 1, base->bodyBones);
}

/** Where body bone b starts, as a fraction of the body from the snout (b >= 1). */
static f32 Rig_BoneStart(FishRigBase* base, s32 b) {
    return RIG_PIVOT + (1.0f - RIG_PIVOT) * (b - 1) / base->bodyBones;
}

static s32 Rig_BuildBase(s32 model, s32 variant, u8* partMap) {
    FishRigBase* base = &sRigBases[model][variant];
    FlexSkeletonHeader* skel = SEGMENTED_TO_VIRTUAL(Rig_Skeleton(model));
    void** limbs = SEGMENTED_TO_VIRTUAL(skel->sh.segment);
    FishRigModelDef* def = &sRigModels[model];
    Vec3f ones[EN_FISHMOD_MAX_LIMBS];
    Vec3f axis;
    Vec3f up;
    f32 sMin;
    f32 sMax;
    s32 gfxCount;
    s32 i;
    s32 k;
    Gfx* out;
    u8 lastPart = 0xFE;

    bzero(base, sizeof(*base));
    base->limbCount = CLAMP_MAX(skel->sh.limbCount, EN_FISHMOD_MAX_LIMBS);
    for (i = 0; i < base->limbCount; i++) {
        base->parent[i] = 0xFF;
        ones[i].x = ones[i].y = ones[i].z = 1.0f;
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

    base->bodyBones = def->bodyBones[variant];
    base->tailLimb = def->tailLimb;
    base->bendLimb = def->bendLimb;
    base->kept[0] = true;
    if (variant == FISH_RIG_FULL) {
        for (k = 0; (k < RIG_MAX_FINS) && (def->finLimbs[k] != 0); k++) {
            base->finLimb[k] = def->finLimbs[k] - 1;
            base->finAnim[k] = def->finAnims[k];
            base->kept[base->finLimb[k]] = true;
        }
        base->finCount = k;
    }
    base->boneCount = 1 + base->bodyBones + base->finCount;
    for (i = 0; i < base->limbCount; i++) {
        s32 l = i;

        while ((l != 0xFF) && (l != 0)) {
            for (k = 0; k < base->finCount; k++) {
                if (base->finLimb[k] == l) {
                    base->finOf[i] = k + 1;
                }
            }
            if (base->finOf[i] != 0) {
                break;
            }
            l = base->parent[l];
        }
    }

    Rig_ListDLs(base, limbs, 0);

    // the vertex loads (and how much display list the copy needs: a bone change can split a load)
    gfxCount = 1;
    for (i = 0; i < base->dlCount; i++) {
        s32 limbIndex = base->dlLimb[i];
        s32 matLimb = limbIndex;
        Gfx* g = Rig_LimbDL(model, SEGMENTED_TO_VIRTUAL(limbs[limbIndex]));

        for (; (g->words.w0 >> 24) != G_ENDDL; g++) {
            u8 op = g->words.w0 >> 24;

            gfxCount++;
            if ((op == G_MTX) && ((g->words.w1 >> 24) == RIG_SEG_MTX)) {
                s32 k2 = (g->words.w1 & 0xFFFFFF) / sizeof(Mtx);

                matLimb = (k2 < base->dlCount) ? base->dlLimb[k2] : limbIndex;
            } else if (op == G_VTX) {
                FishRigVtxLoad* load;

                if (base->loadCount >= RIG_MAX_LOADS) {
                    return false;
                }
                load = &base->loads[base->loadCount++];
                load->src = SEGMENTED_TO_VIRTUAL(g->words.w1);
                load->n = (g->words.w0 >> 12) & 0xFF;
                load->dst = base->vtxCount;
                load->limb = matLimb;
                base->vtxCount += load->n;
                gfxCount += 2 * load->n;
            }
        }
        gfxCount += 3;
    }

    // which bone every vertex goes with (from the unscaled model, so all species share the display list)
    base->vtxBone = ZeldaArena_Malloc(base->vtxCount);
    base->dl = ZeldaArena_Malloc(gfxCount * sizeof(Gfx));
    if ((base->vtxBone == NULL) || (base->dl == NULL)) {
        return false;
    }
    Rig_Spine(base, ones, &axis, &up, &sMin, &sMax);
    for (i = 0; i < base->loadCount; i++) {
        FishRigVtxLoad* load = &base->loads[i];
        RigMtx f;
        s32 v;

        Rig_StaticFrame(base, ones, load->limb, &f);
        for (v = 0; v < load->n; v++) {
            Vec3f p;
            Vec3f q;

            if (base->finOf[load->limb] != 0) {
                base->vtxBone[load->dst + v] = 1 + base->bodyBones + base->finOf[load->limb] - 1;
                continue;
            }
            p.x = load->src[v].n.ob[0];
            p.y = load->src[v].n.ob[1];
            p.z = load->src[v].n.ob[2];
            Rig_MulPoint(&f, &p, &q);
            base->vtxBone[load->dst + v] =
                Rig_BoneAt(base, ((q.x * axis.x + q.y * axis.y + q.z * axis.z) - sMin) / CLAMP_MIN(sMax - sMin, 1.0f));
        }
    }

    // the shared display list: per limb a tint and a copy of its display list, with a matrix load wherever the bone
    // changes (a vertex load that spans bones is split)
    out = base->dl;
    k = 0; // load index
    for (i = 0; i < base->dlCount; i++) {
        s32 limbIndex = base->dlLimb[i];
        Gfx* g = Rig_LimbDL(model, SEGMENTED_TO_VIRTUAL(limbs[limbIndex]));
        u8 part = partMap[limbIndex];
        s32 curBone = -1;

        if ((part != FISHMOD_PART_NONE) && (part != lastPart)) {
            gSPDisplayList(out++, SEGMENT_ADDR(RIG_SEG_TINT, part * 2 * sizeof(Gfx)));
            lastPart = part;
        }
        for (; (g->words.w0 >> 24) != G_ENDDL; g++) {
            u8 op = g->words.w0 >> 24;

            if ((op == G_MTX) && ((g->words.w1 >> 24) == RIG_SEG_MTX)) {
                continue; // the model's own seams: every vertex now has its bone
            }
            if (op == G_VTX) {
                FishRigVtxLoad* load = &base->loads[k++];
                s32 v0 = ((g->words.w0 >> 1) & 0x7F) - load->n;
                s32 a = 0;

                while (a < load->n) {
                    s32 bone = base->vtxBone[load->dst + a];
                    s32 b = a + 1;

                    while ((b < load->n) && (base->vtxBone[load->dst + b] == bone)) {
                        b++;
                    }
                    if (bone != curBone) {
                        gSPMatrix(out++, SEGMENT_ADDR(RIG_SEG_MTX, bone * sizeof(Mtx)),
                                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                        curBone = bone;
                    }
                    gSPVertex(out++, SEGMENT_ADDR(RIG_SEG_VTX, (load->dst + a) * sizeof(Vtx)), b - a, v0 + a);
                    a = b;
                }
                continue;
            }
            *out++ = *g;
        }
    }
    gSPEndDisplayList(out++);
    base->ready = true;
    return true;
}

static FishRigSpecies* Rig_BuildSpecies(const FishmodSpecies* species, s32 model, s32 variant, u8* partMap) {
    FishRigBase* base = &sRigBases[model][variant];
    FishRigSpecies* rig;
    Vec3f scales[EN_FISHMOD_MAX_LIMBS];
    f32 sMin;
    f32 sMax;
    f32 len;
    s32 i;
    s32 b;

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

    // the spine of this species' shape, and the bones along it
    Rig_Spine(base, scales, &rig->axis, &rig->up, &sMin, &sMax);
    len = CLAMP_MIN(sMax - sMin, 1.0f);
    rig->bonePos[0].x = rig->axis.x * (sMin + len * RIG_PIVOT);
    rig->bonePos[0].y = rig->axis.y * (sMin + len * RIG_PIVOT);
    rig->bonePos[0].z = rig->axis.z * (sMin + len * RIG_PIVOT);
    for (b = 1; b <= base->bodyBones; b++) {
        f32 s = sMin + len * Rig_BoneStart(base, b);

        rig->bonePos[b].x = rig->axis.x * s;
        rig->bonePos[b].y = rig->axis.y * s;
        rig->bonePos[b].z = rig->axis.z * s;
    }
    rig->headPoint.x = -rig->bonePos[0].x;
    rig->headPoint.y = -rig->bonePos[0].y;
    rig->headPoint.z = -rig->bonePos[0].z;
    rig->tailPoint.x = rig->axis.x * sMax - rig->bonePos[base->bodyBones].x;
    rig->tailPoint.y = rig->axis.y * sMax - rig->bonePos[base->bodyBones].y;
    rig->tailPoint.z = rig->axis.z * sMax - rig->bonePos[base->bodyBones].z;

    Rig_Identity(&rig->headFrame);
    Rig_Translate(&rig->headFrame, base->pose[0].x, base->pose[0].y, base->pose[0].z);
    Rig_RotateZYX(&rig->headFrame, base->pose[1].x, base->pose[1].y, base->pose[1].z);

    // fins: hung on the body bone at their joint
    for (i = 0; i < base->finCount; i++) {
        s32 limb = base->finLimb[i];
        s32 p = base->parent[limb];
        RigMtx f;
        f32 s;

        Rig_StaticFrame(base, scales, p, &f);
        Rig_Translate(&f, base->jointPos[limb].x * scales[p].x, base->jointPos[limb].y * scales[p].y,
                      base->jointPos[limb].z * scales[p].z);
        s = f.m[0][3] * rig->axis.x + f.m[1][3] * rig->axis.y + f.m[2][3] * rig->axis.z;
        rig->finParent[i] = Rig_BoneAt(base, (s - sMin) / len);
        rig->finPre[i] = f;
        rig->finPre[i].m[0][3] -= rig->bonePos[rig->finParent[i]].x;
        rig->finPre[i].m[1][3] -= rig->bonePos[rig->finParent[i]].y;
        rig->finPre[i].m[2][3] -= rig->bonePos[rig->finParent[i]].z;
    }

    // bake the vertices: rest pose and part scale, relative to their bone
    for (i = 0; i < base->loadCount; i++) {
        FishRigVtxLoad* load = &base->loads[i];
        RigMtx f;
        s32 v;

        Rig_StaticFrame(base, scales, load->limb, &f);
        Rig_Scale(&f, scales[load->limb].x, scales[load->limb].y, scales[load->limb].z);
        for (v = 0; v < load->n; v++) {
            Vtx* src = &load->src[v];
            Vtx* dst = &rig->vtx[load->dst + v];
            s32 bone = base->vtxBone[load->dst + v];
            Vec3f p;
            Vec3f q;
            s32 c;

            *dst = *src;
            p.x = src->n.ob[0];
            p.y = src->n.ob[1];
            p.z = src->n.ob[2];
            Rig_MulPoint(&f, &p, &q);
            if (bone <= base->bodyBones) {
                // body bones sit on the spine, square with the head
                q.x -= rig->bonePos[bone].x;
                q.y -= rig->bonePos[bone].y;
                q.z -= rig->bonePos[bone].z;
            }
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
            if (sRigBases[i][v].vtxBone != NULL) {
                ZeldaArena_Free(sRigBases[i][v].vtxBone);
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
 * The body's heading at a place along it (fraction from the snout), relative to its heading at the pivot, in binary
 * angle units: the slope of the body wave y = A(x) sin(kx - phase), plus the curve of a turn or a startle.
 */
static s16 Rig_BodyAngle(FishRigSwim* swim, f32 x, f32 phase, f32 amp, f32 curve) {
    f32 k = 2.0f * M_PI / swim->wavelength;
    f32 a;
    f32 da;
    f32 slope;
    f32 slope0;
    s16 ph = (s16)((k * x - phase) * (0x8000 / M_PI));
    s16 ph0 = (s16)((k * RIG_PIVOT - phase) * (0x8000 / M_PI));

    a = swim->a0 + swim->a1 * x + swim->a2 * x * x;
    da = swim->a1 + 2.0f * swim->a2 * x;
    slope = da * Math_SinS(ph) + a * k * Math_CosS(ph);
    a = swim->a0 + swim->a1 * RIG_PIVOT + swim->a2 * RIG_PIVOT * RIG_PIVOT;
    da = swim->a1 + 2.0f * swim->a2 * RIG_PIVOT;
    slope0 = da * Math_SinS(ph0) + a * k * Math_CosS(ph0);
    return (s16)(((slope - slope0) * amp + curve * (x - RIG_PIVOT)) * (0x8000 / M_PI));
}

/**
 * Draws a fish with its rig (EnFishmod_RigSetSpecies first). Also updates its head position, and its bounds when
 * something needs them (held up by Link, photos, whiskers).
 */
void EnFishmod_RigDraw(EnFishmod* this, PlayState* play, s32 variant, s32 wantBounds) {
    s32 model = EnFishmod_RigModel(this->species);
    FishRigBase* base = &sRigBases[model][variant];
    FishRigSpecies* rig = sRigSpecies[this->species - gFishmodSpecies][variant];
    FishRigSwim* swim = &sRigModels[model].swim;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Mtx* mtx = GRAPH_ALLOC(gfxCtx, base->boneCount * sizeof(Mtx));
    RigMtx frames[FISH_RIG_MAX_SLOTS];
    RigMtx root;
    Vec3f pivot;
    const u8* shape = this->species->shape;
    // the wave's size from the swim strength (1500 = cruising); small fish move bigger
    f32 amp = this->swimMag * (1.0f / 1500.0f) *
              ((this->lengthInches < FISHMOD_SMALL_RIG_LENGTH) ? 1.3f : 1.0f);
    // turns and startles bend the whole body into a curve (radians across the body behind the pivot)
    f32 curve = (-this->turnBend * 3.0f + this->startleBend * 5.4f) * (M_PI / 0x8000);
    s16 head;
    s16 prev;
    s32 b;
#if FISHMOD_DEBUG
    OSTime profStart = osGetTime();
#endif

    // sprints and thrashing beat faster, not much wider
    if (amp > 1.0f) {
        amp = 1.0f + (amp - 1.0f) * 0.35f;
    }

    // the fish's root: placed so that it turns about the pivot (about a third of the way back from the snout)
    Rig_Identity(&root);
    root.m[0][3] = this->actor.world.pos.x;
    root.m[1][3] = this->actor.world.pos.y;
    root.m[2][3] = this->actor.world.pos.z;
    Rig_RotateY(&root, this->actor.shape.rot.y);
    Rig_RotateX(&root, this->actor.shape.rot.x);
    Rig_RotateZ(&root, this->actor.shape.rot.z);
    Rig_Scale(&root, this->actor.scale.x, this->actor.scale.y, this->actor.scale.z);
    if (model != FISH_RIG_SMALL) {
        Rig_RotateY(&root, -0x4000);
    }
    Rig_Scale(&root, shape[0] * 0.01f, shape[1] * 0.01f, shape[2] * 0.01f);
    Rig_MulPoint(&rig->headFrame, &rig->bonePos[0], &pivot);
    Rig_Translate(&root, -pivot.x, -pivot.y, -pivot.z);
    Rig_Mul(&root, &root, &rig->headFrame);
#if FISHMOD_DEBUG
    gFishmodProf[FISHMOD_PROF_D_ROOT] += osGetTime() - profStart;
    gFishmodProfCount[FISHMOD_PROF_D_ROOT]++;
    profStart = osGetTime();
#endif

    // the head turns about the pivot against the tail (recoil); the body bones follow the wave from the pivot back
    frames[0] = root;
    Rig_Translate(&frames[0], rig->bonePos[0].x, rig->bonePos[0].y, rig->bonePos[0].z);
    head = Rig_BodyAngle(swim, RIG_PIVOT * 0.4f, this->swimPhase, amp, curve);
    Rig_RotateAxis(&frames[0], &rig->up, head);
    prev = head;
    for (b = 1; b <= base->bodyBones; b++) {
        f32 mid = Rig_BoneStart(base, b) + (1.0f - RIG_PIVOT) * 0.5f / base->bodyBones;
        s16 angle = Rig_BodyAngle(swim, mid, this->swimPhase, amp, curve);

        if (b == 1) {
            frames[1] = root;
            Rig_Translate(&frames[1], rig->bonePos[1].x, rig->bonePos[1].y, rig->bonePos[1].z);
        } else {
            frames[b] = frames[b - 1];
            Rig_Translate(&frames[b], rig->bonePos[b].x - rig->bonePos[b - 1].x,
                          rig->bonePos[b].y - rig->bonePos[b - 1].y, rig->bonePos[b].z - rig->bonePos[b - 1].z);
        }
        // no sharper than this between neighbouring bones: past it a wide body folds instead of bending
        // (half that behind the head, where the body is widest)
        {
            s32 maxJoint = (b == 1) ? (RIG_MAX_JOINT / 2) : RIG_MAX_JOINT;

            angle = CLAMP(angle, prev - maxJoint, prev + maxJoint);
        }
        Rig_RotateAxis(&frames[b], &rig->up, (b == 1) ? angle : (s16)(angle - prev));
        prev = angle;
    }
    for (b = 0; b < base->finCount; b++) {
        s32 slot = 1 + base->bodyBones + b;
        Vec3s* rot = &base->pose[base->finLimb[b] + 1];
        s16 dy = 0;
        s16 dz = 0;

        switch (base->finAnim[b]) {
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
        Rig_Mul(&frames[slot], &frames[rig->finParent[b]], &rig->finPre[b]);
        Rig_RotateZYX(&frames[slot], rot->x, rot->y + dy, rot->z + dz);
    }
    for (b = 0; b < base->boneCount; b++) {
        Rig_ToMtx(&frames[b], &mtx[b]);
    }

    Rig_MulPoint(&frames[0], &rig->headPoint, &this->headPos);
    if (wantBounds) {
        Vec3f p;

        this->boundsMin = this->boundsMax = this->headPos;
        for (b = 0; b <= base->bodyBones + 1; b++) {
            if (b <= base->bodyBones) {
                p.x = frames[b].m[0][3];
                p.y = frames[b].m[1][3];
                p.z = frames[b].m[2][3];
            } else {
                Rig_MulPoint(&frames[base->bodyBones], &rig->tailPoint, &p);
            }
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
