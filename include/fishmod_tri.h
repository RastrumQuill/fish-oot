#ifndef FISHMOD_TRI_H
#define FISHMOD_TRI_H

/*
 * fish-oot: Math3D_TriChkPointParaYImpl's test (is the point (z, x) within chkDist of the triangle, seen from above),
 * the same answers for every input, cheaper:
 * - its three tests (near a corner; inside, by the determinants; near an edge) are or-ed and have no side effects, so
 *   the cheap determinant test goes first: a point inside the triangle (a floor under an actor) returns before the
 *   corner distances;
 * - an edge's distance is computed only when the point is beside the edge (Math3D_PointDistSqToLine2D computes it
 *   either way, then the caller throws it away);
 * - static, so a caller can have it inlined (no call, its arguments in registers), and a caller that has already
 *   made the bounding-box test (the scene's floor and ceiling searches, from each poly's stored box) skips it.
 * Every expression is the original's, in the original's order within it: the same floats.
 */

#include "z_math.h"

/* Math3D_PointDistSqToLine2D, but only when the point is beside the segment (ratio in [0, 1]): its distance then, else
   false (the original's distance in that case was never used by the triangle tests) */
static s32 Fishmod_PointDistSqToSeg2D(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32* lineLenSq) {
    f32 xDiff = x2 - x1;
    f32 yDiff = y2 - y1;
    f32 distSq = SQ(xDiff) + SQ(yDiff);
    f32 perpendicularRatio;
    f32 px;
    f32 py;

    if (IS_ZERO(distSq)) {
        return false;
    }
    perpendicularRatio = (((x0 - x1) * xDiff) + (y0 - y1) * yDiff) / distSq;
    if (!(perpendicularRatio >= 0.0f && perpendicularRatio <= 1.0f)) {
        return false;
    }
    px = (xDiff * perpendicularRatio) + x1;
    py = (yDiff * perpendicularRatio) + y1;
    *lineLenSq = SQ(px - x0) + SQ(py - y0);
    return true;
}

/* Math3D_TriChkPointParaYImpl's answer; boxDone: the caller has already made its bounding-box test */
static s32 Fishmod_TriChkPointParaY(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 z, f32 x, f32 detMax, f32 chkDist, f32 ny,
                                    s32 boxDone) {
    f32 detv0v1;
    f32 detv1v2;
    f32 detv2v0;
    f32 distToEdgeSq;
    f32 chkDistSq;

    if (!boxDone) {
        /* (Math3D_CirSquareVsTriSquare: the triangle's box, widened by chkDist, against the point) */
        f32 minX = v0->z;
        f32 maxX = v0->z;
        f32 minY = v0->x;
        f32 maxY = v0->x;

        if (v1->z < minX) {
            minX = v1->z;
        } else if (maxX < v1->z) {
            maxX = v1->z;
        }
        if (v1->x < minY) {
            minY = v1->x;
        } else if (maxY < v1->x) {
            maxY = v1->x;
        }
        if (v2->z < minX) {
            minX = v2->z;
        } else if (maxX < v2->z) {
            maxX = v2->z;
        }
        if (v2->x < minY) {
            minY = v2->x;
        } else if (maxY < v2->x) {
            maxY = v2->x;
        }
        if (!((minX - chkDist) <= z && (maxX + chkDist) >= z && (minY - chkDist) <= x && (maxY + chkDist) >= x)) {
            return false;
        }
    }

    detv0v1 = ((v0->z - z) * (v1->x - x)) - ((v0->x - x) * (v1->z - z));
    detv1v2 = ((v1->z - z) * (v2->x - x)) - ((v1->x - x) * (v2->z - z));
    detv2v0 = ((v2->z - z) * (v0->x - x)) - ((v2->x - x) * (v0->z - z));
    if (((detMax >= detv0v1) && (detMax >= detv1v2) && (detMax >= detv2v0)) ||
        ((-detMax <= detv0v1) && (-detMax <= detv1v2) && (-detMax <= detv2v0))) {
        return true;
    }

    chkDistSq = SQ(chkDist);
    if (((SQ(v0->z - z) + SQ(v0->x - x)) < chkDistSq) || ((SQ(v1->z - z) + SQ(v1->x - x)) < chkDistSq) ||
        ((SQ(v2->z - z) + SQ(v2->x - x)) < chkDistSq)) {
        return true;
    }

    if (fabsf(ny) > 0.5f) {
        if (Fishmod_PointDistSqToSeg2D(z, x, v0->z, v0->x, v1->z, v1->x, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
            return true;
        }
        if (Fishmod_PointDistSqToSeg2D(z, x, v1->z, v1->x, v2->z, v2->x, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
            return true;
        }
        if (Fishmod_PointDistSqToSeg2D(z, x, v2->z, v2->x, v0->z, v0->x, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
            return true;
        }
    }
    return false;
}

#endif
