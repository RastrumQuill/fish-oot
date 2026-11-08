/*
 * The occlusion plane (F3DEX3's, the variant built with it: the build profile's f3dex3_occlusion). Each frame the best
 * of the scene's occluder candidates (fishmod/tools/gen_occluders.py: flat quads just inside solid ground or walls) is
 * turned into the microcode's coefficients, and the RSP drops the props' triangles that are entirely behind it, before
 * the RDP spends a span on them.
 *
 * Written from the microcode's own description of its test (f3dex3.s, occlusionPlaneEdgeCoeffs). A vertex is hidden
 * when all five hold, with u, v its screen position in quarter pixels (as the RSP has it):
 *   v > 8 c0 u - 2 c4    (top)        u > 8 c1 v - 2 c5    (left)
 *   v < -8 c2 u + 2 c6   (bottom)     u < -8 c3 v + 2 c7   (right)
 *   0.5 (kx X + ky Y + kz Z) >= kc    (behind the plane; X, Y, Z its clip-space position, k as fractions)
 * c0-c3, kx-kz are s0.15 fractions (so the four lines' slopes are within +-8), c4-c7 and kc plain s16. A triangle is
 * dropped when all three of its vertices are hidden (never one that needs clipping).
 *
 * Here: the four lines are fitted inside the occluder's outline on screen (each on the matching edge of the outline,
 * its slope within reach, then moved in until the region's four corners are all inside the outline: a convex region
 * whose corners are inside a convex outline is inside it, so nothing the occluder doesn't cover is ever dropped), and
 * the plane is the occluder's own, through its corners in clip space (the perspective map is affine in X, Y, Z, so a
 * plane stays a plane), turned so the camera's side is the kept one.
 */
#include "z_en_fishmod.h"
#include "fishmod.h"
#include "gfx.h"
#include "play_state.h"
#include "libc/math.h"
#include "ultra64.h"
#include "src/fishmod_data/fishmod_profile.h"
#include "array_count.h"

#if FISHMOD_PROFILE_OCCLUSION && defined(F3DEX3_OCCLUSION)

#include "src/fishmod_data/fishmod_occluders.h"

typedef struct OccCandidate {
    s16 sceneId;
    s16 v[4][3];
    s16 n[3]; // outward normal x 1000: the camera's side
} OccCandidate;

#define X(scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, nx, ny, nz) \
    { scene, { { x0, y0, z0 }, { x1, y1, z1 }, { x2, y2, z2 }, { x3, y3, z3 } }, { nx, ny, nz } },
static const OccCandidate sCandidates[] = { FISHMOD_OCCLUDERS(X){ -1, { { 0 } }, { 0 } } }; // (an end mark)
#undef X

// the coefficients that hide nothing (the microcode's own defaults)
static OcclusionPlane sOccNone = { { 0, 0, 0, 0, (s16)0x8000, (s16)0x8000, (s16)0x8000, (s16)0x8000, 0, 0, 0,
                                     0x7FFF } };

#define OCC_MIN_W 30.0f      // every corner this far in front of the camera, or the candidate isn't used
#define OCC_MAX_DIST 3000.0f // candidates further than this aren't looked at
#define OCC_SLOPE_MAX 7.9f   // the lines' slopes (8 is the limit: c in s0.15)
#define OCC_MARGIN 4.0f      // a pixel inside the outline (quarter pixels)
#define OCC_STEPS 64

typedef struct OccLine {
    f32 a; // slope: for top/bottom dv/du, for left/right du/dv
    f32 b;
} OccLine;

static void Occ_Transform(MtxF* m, f32 x, f32 y, f32 z, f32* out) {
    out[0] = m->xx * x + m->xy * y + m->xz * z + m->xw;
    out[1] = m->yx * x + m->yy * y + m->yz * z + m->yw;
    out[2] = m->zx * x + m->zy * y + m->zz * z + m->zw;
    out[3] = m->wx * x + m->wy * y + m->wz * z + m->ww;
}

/** Is (u, v) inside the convex polygon (ccw or cw, given by sign), at least margin in from every edge? */
static s32 Occ_Inside(f32 (*p)[2], s32 n, f32 sign, f32 u, f32 v) {
    s32 i;

    for (i = 0; i < n; i++) {
        f32* a = p[i];
        f32* b = p[(i + 1) % n];
        f32 ex = b[0] - a[0];
        f32 ey = b[1] - a[1];
        f32 len = sqrtf(SQ(ex) + SQ(ey));
        f32 cross = (ex * (v - a[1]) - ey * (u - a[0])) * sign;

        if ((len > 0.0f) && (cross < OCC_MARGIN * len)) {
            return false;
        }
    }
    return true;
}

/** Where a top/bottom line (v = a u + b) meets a left/right one (u = a v + b). False if they're nearly parallel. */
static s32 Occ_Corner(OccLine* h, OccLine* s, f32* u, f32* v) {
    f32 den = 1.0f - s->a * h->a;

    if (fabsf(den) < 0.1f) {
        return false;
    }
    *u = (s->a * h->b + s->b) / den;
    *v = h->a * *u + h->b;
    return true;
}

static s16 Occ_Frac(f32 f) {
    f *= 32768.0f;
    return (s16)CLAMP(f, -32768.0f, 32767.0f);
}

/**
 * The coefficients for one candidate as the camera sees it now (vp: the view-projection matrix; vpScale/vpTrans the
 * viewport in quarter pixels). Returns the hidden region's size on screen (quarter pixels squared), or 0 if it can't
 * be used (behind the camera, too edge-on, its region empty, a value out of range).
 */
static f32 Occ_Make(const OccCandidate* c, MtxF* vp, f32* vpScale, f32* vpTrans, Vec3f* eye, OcclusionPlane* out) {
    f32 clip[4][4];
    f32 scr[4][2];
    f32 sign;
    f32 area;
    s32 i;
    OccLine ln[4]; // top, left, bottom, right
    f32 cu[4];
    f32 cv[4];
    s32 step;
    f32 du;
    f32 dv;
    f32 size;

    // the camera on the outward side, a little off the plane
    if (((eye->x - c->v[0][0]) * c->n[0] + (eye->y - c->v[0][1]) * c->n[1] + (eye->z - c->v[0][2]) * c->n[2]) <
        20.0f * 1000.0f) {
        return 0.0f;
    }
    for (i = 0; i < 4; i++) {
        Occ_Transform(vp, c->v[i][0], c->v[i][1], c->v[i][2], clip[i]);
        if (clip[i][3] < OCC_MIN_W) {
            return 0.0f;
        }
        scr[i][0] = clip[i][0] / clip[i][3] * vpScale[0] + vpTrans[0];
        scr[i][1] = clip[i][1] / clip[i][3] * vpScale[1] + vpTrans[1];
    }
    // the outline's winding (signed area)
    area = 0.0f;
    for (i = 0; i < 4; i++) {
        area += scr[i][0] * scr[(i + 1) % 4][1] - scr[(i + 1) % 4][0] * scr[i][1];
    }
    sign = (area >= 0.0f) ? 1.0f : -1.0f;
    if (fabsf(area) < 2.0f * 64.0f * 64.0f) { // under about 16 x 16 pixels: not worth it
        return 0.0f;
    }
    du = dv = 0.0f;
    {
        f32 uMin = scr[0][0], uMax = scr[0][0], vMin = scr[0][1], vMax = scr[0][1];

        for (i = 1; i < 4; i++) {
            uMin = MIN(uMin, scr[i][0]);
            uMax = MAX(uMax, scr[i][0]);
            vMin = MIN(vMin, scr[i][1]);
            vMax = MAX(vMax, scr[i][1]);
        }
        size = MAX(uMax - uMin, vMax - vMin);
        // each line starts on the outline's edge facing its way (the edge whose outward normal points most that way)
        {
            static const f32 sDir[4][2] = { { 0.0f, -1.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f } };
            s32 k;

            for (k = 0; k < 4; k++) {
                f32 best = -2.0f;
                s32 e = 0;

                for (i = 0; i < 4; i++) {
                    f32 ex = scr[(i + 1) % 4][0] - scr[i][0];
                    f32 ey = scr[(i + 1) % 4][1] - scr[i][1];
                    f32 len = sqrtf(SQ(ex) + SQ(ey));
                    // outward normal: (ey, -ex) * sign for a ccw outline in these axes
                    f32 nx = ey * sign / len;
                    f32 ny = -ex * sign / len;
                    f32 d = nx * sDir[k][0] + ny * sDir[k][1];

                    if ((len > 0.0f) && (d > best)) {
                        best = d;
                        e = i;
                    }
                }
                {
                    f32* p0 = scr[e];
                    f32* p1 = scr[(e + 1) % 4];
                    f32 mu = (p0[0] + p1[0]) * 0.5f;
                    f32 mv = (p0[1] + p1[1]) * 0.5f;

                    if ((k & 1) == 0) { // top/bottom: v = a u + b
                        f32 dd = p1[0] - p0[0];
                        ln[k].a = (fabsf(dd) > 0.01f) ? (p1[1] - p0[1]) / dd : 0.0f;
                        ln[k].a = CLAMP(ln[k].a, -OCC_SLOPE_MAX, OCC_SLOPE_MAX);
                        ln[k].b = mv - ln[k].a * mu;
                    } else { // left/right: u = a v + b
                        f32 dd = p1[1] - p0[1];
                        ln[k].a = (fabsf(dd) > 0.01f) ? (p1[0] - p0[0]) / dd : 0.0f;
                        ln[k].a = CLAMP(ln[k].a, -OCC_SLOPE_MAX, OCC_SLOPE_MAX);
                        ln[k].b = mu - ln[k].a * mv;
                    }
                }
            }
        }
    }
    // move the lines in until the region's corners are all inside the outline
    {
        f32 stepSize = MAX(2.0f, size / 48.0f);

        for (step = 0; step < OCC_STEPS; step++) {
            s32 ok = true;

            // corners: 0 top-left, 1 top-right, 2 bottom-right, 3 bottom-left
            if (!Occ_Corner(&ln[0], &ln[1], &cu[0], &cv[0]) || !Occ_Corner(&ln[0], &ln[3], &cu[1], &cv[1]) ||
                !Occ_Corner(&ln[2], &ln[3], &cu[2], &cv[2]) || !Occ_Corner(&ln[2], &ln[1], &cu[3], &cv[3])) {
                return 0.0f;
            }
            // the region must still be a region (top above bottom, left of right)
            if ((cv[0] >= cv[3]) || (cv[1] >= cv[2]) || (cu[0] >= cu[1]) || (cu[3] >= cu[2])) {
                return 0.0f;
            }
            for (i = 0; i < 4; i++) {
                if (!Occ_Inside(scr, 4, sign, cu[i], cv[i])) {
                    ok = false;
                    // in with the two lines that meet there (top/bottom b moves v, left/right b moves u)
                    ln[(i < 2) ? 0 : 2].b += (i < 2) ? stepSize : -stepSize;
                    ln[((i == 0) || (i == 3)) ? 1 : 3].b += ((i == 0) || (i == 3)) ? stepSize : -stepSize;
                }
            }
            if (ok) {
                break;
            }
        }
        if (step == OCC_STEPS) {
            return 0.0f;
        }
    }
    // the plane through three corners in clip space, the camera's side kept
    {
        f32 ax = clip[1][0] - clip[0][0], ay = clip[1][1] - clip[0][1], az = clip[1][2] - clip[0][2];
        f32 bx = clip[2][0] - clip[0][0], by = clip[2][1] - clip[0][1], bz = clip[2][2] - clip[0][2];
        f32 nx = ay * bz - az * by;
        f32 ny = az * bx - ax * bz;
        f32 nz = ax * by - ay * bx;
        f32 d = -(nx * clip[0][0] + ny * clip[0][1] + nz * clip[0][2]);
        f32 behind[4];
        f32 m;
        f32 s;

        // a point behind the occluder (its centre, pushed back against its normal) must test as behind
        Occ_Transform(vp,
                      (c->v[0][0] + c->v[2][0]) * 0.5f - c->n[0] * 0.05f,
                      (c->v[0][1] + c->v[2][1]) * 0.5f - c->n[1] * 0.05f,
                      (c->v[0][2] + c->v[2][2]) * 0.5f - c->n[2] * 0.05f, behind);
        if (nx * behind[0] + ny * behind[1] + nz * behind[2] + d < 0.0f) {
            nx = -nx;
            ny = -ny;
            nz = -nz;
            d = -d;
        }
        m = MAX(fabsf(nx), MAX(fabsf(ny), fabsf(nz)));
        if (m < 1e-6f) {
            return 0.0f;
        }
        s = 0.99f / m;
        // hidden when 0.5 (kx X + ky Y + kz Z) >= kc: kc = -0.5 d
        if (fabsf(d * s * 0.5f) > 32000.0f) {
            return 0.0f;
        }
        out->o.kx = Occ_Frac(nx * s);
        out->o.ky = Occ_Frac(ny * s);
        out->o.kz = Occ_Frac(nz * s);
        out->o.kc = (s16)(-0.5f * d * s);
    }
    // the four lines as coefficients (the bounds' b must fit, or the region would grow when clamped)
    for (i = 0; i < 4; i++) {
        if (fabsf(ln[i].b) * 0.5f > 32000.0f) {
            return 0.0f;
        }
    }
    out->o.c0 = Occ_Frac(ln[0].a / 8.0f);
    out->o.c4 = (s16)(-ln[0].b * 0.5f);
    out->o.c1 = Occ_Frac(ln[1].a / 8.0f);
    out->o.c5 = (s16)(-ln[1].b * 0.5f);
    out->o.c2 = Occ_Frac(-ln[2].a / 8.0f);
    out->o.c6 = (s16)(ln[2].b * 0.5f);
    out->o.c3 = Occ_Frac(-ln[3].a / 8.0f);
    out->o.c7 = (s16)(ln[3].b * 0.5f);
    // the region's size (two triangles of its corners)
    return 0.5f * fabsf((cu[1] - cu[0]) * (cv[3] - cv[0]) - (cu[3] - cu[0]) * (cv[1] - cv[0])) +
           0.5f * fabsf((cu[1] - cu[2]) * (cv[3] - cv[2]) - (cu[3] - cu[2]) * (cv[1] - cv[2]));
}

static s32 sOccChosen = -1; // (for the debug readout)

/** The occlusion plane for this frame (in the frame's display memory), or the one that hides nothing. */
void* EnFishmod_OcclusionPlane(PlayState* play) {
    OcclusionPlane* best = &sOccNone;
    OcclusionPlane* work;
    f32 bestArea = 0.0f;
    f32 vpScale[2];
    f32 vpTrans[2];
    Vec3f* eye = &play->view.eye;
    s32 i;

    sOccChosen = -1;
#if FISHMOD_DEBUG
    if (gFishmodBench & FISHMOD_BENCH_NOOCC) {
        return &sOccNone;
    }
#endif
    work = GRAPH_ALLOC(play->state.gfxCtx, 2 * sizeof(OcclusionPlane));
    // the screen position as the RSP computes it: the divided clip position times the viewport's scale, plus its
    // offset (both kept in quarter pixels: the units of the test)
    vpScale[0] = play->view.vp.vp.vscale[0];
    vpScale[1] = play->view.vp.vp.vscale[1];
    vpTrans[0] = play->view.vp.vp.vtrans[0];
    vpTrans[1] = play->view.vp.vp.vtrans[1];
    for (i = 0; i < ARRAY_COUNT(sCandidates); i++) {
        const OccCandidate* c = &sCandidates[i];
        OcclusionPlane* slot = (best == &work[0]) ? &work[1] : &work[0];
        f32 dx, dz, a;

        if (c->sceneId != play->sceneId) {
            continue;
        }
        dx = c->v[0][0] - eye->x;
        dz = c->v[0][2] - eye->z;
        if (SQ(dx) + SQ(dz) > SQ(OCC_MAX_DIST)) {
            continue;
        }
        a = Occ_Make(c, &play->viewProjectionMtxF, vpScale, vpTrans, eye, slot);
        if (a > bestArea) {
            bestArea = a;
            best = slot;
            sOccChosen = i;
        }
    }
    return best;
}

void* EnFishmod_OcclusionNone(void) {
    return &sOccNone;
}

s32 EnFishmod_OcclusionChosen(void) {
    return sOccChosen;
}

#endif
