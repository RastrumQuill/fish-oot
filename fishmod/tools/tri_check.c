/*
 * Checks include/fishmod_tri.h (the cheaper Math3D_TriChkPointParaYImpl) against the original, copied verbatim
 * below, on random triangles and points: anywhere near, on a corner, on an edge, just off an edge; every tolerance
 * the game uses (chkDist 0-50, detMax 0 and 300). Every answer must be the same.
 *
 * Build and run (from the repository root):
 *   gcc -O2 -ffp-contract=off -D_LANGUAGE_C -Iinclude -o /tmp/tri_check fishmod/tools/tri_check.c -lm && /tmp/tri_check
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64/ultratypes.h"
typedef struct { f32 x, y, z; } Vec3f;
#define SQ(x) ((x) * (x))
#define Z_MATH_H
#define IS_ZERO(f) (fabsf(f) < 0.008f)
#define true 1
#define false 0
#include "fishmod_tri.h"
/* the originals, verbatim */
static s32 O_Cir(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 centerX, f32 centerY, f32 radius) {
    f32 minX, maxX, minY, maxY;
    minX = maxX = x0; minY = maxY = y0;
    if (x1 < minX) minX = x1; else if (maxX < x1) maxX = x1;
    if (y1 < minY) minY = y1; else if (maxY < y1) maxY = y1;
    if (x2 < minX) minX = x2; else if (maxX < x2) maxX = x2;
    if (y2 < minY) minY = y2; else if (maxY < y2) maxY = y2;
    return (minX - radius) <= centerX && (maxX + radius) >= centerX && (minY - radius) <= centerY && (maxY + radius) >= centerY;
}
static s32 O_Dist(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32* lineLenSq) {
    f32 r, xd = x2 - x1, yd = y2 - y1, d = SQ(xd) + SQ(yd), px, py; s32 ret = 0;
    if (IS_ZERO(d)) { *lineLenSq = 0.0f; return 0; }
    r = (((x0 - x1) * xd) + (y0 - y1) * yd) / d;
    if (r >= 0.0f && r <= 1.0f) ret = 1;
    px = (xd * r) + x1; py = (yd * r) + y1;
    *lineLenSq = SQ(px - x0) + SQ(py - y0);
    return ret;
}
static s32 O_Impl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 z, f32 x, f32 detMax, f32 chkDist, f32 ny) {
    f32 a, b, c, e, k;
    if (!O_Cir(v0->z, v0->x, v1->z, v1->x, v2->z, v2->x, z, x, chkDist)) return 0;
    k = SQ(chkDist);
    if (((SQ(v0->z - z) + SQ(v0->x - x)) < k) || ((SQ(v1->z - z) + SQ(v1->x - x)) < k) || ((SQ(v2->z - z) + SQ(v2->x - x)) < k)) return 1;
    a = ((v0->z - z) * (v1->x - x)) - ((v0->x - x) * (v1->z - z));
    b = ((v1->z - z) * (v2->x - x)) - ((v1->x - x) * (v2->z - z));
    c = ((v2->z - z) * (v0->x - x)) - ((v2->x - x) * (v0->z - z));
    if (((detMax >= a) && (detMax >= b) && (detMax >= c)) || ((-detMax <= a) && (-detMax <= b) && (-detMax <= c))) return 1;
    if (fabsf(ny) > 0.5f) {
        if (O_Dist(z, x, v0->z, v0->x, v1->z, v1->x, &e) && (e < k)) return 1;
        if (O_Dist(z, x, v1->z, v1->x, v2->z, v2->x, &e) && (e < k)) return 1;
        if (O_Dist(z, x, v2->z, v2->x, v0->z, v0->x, &e) && (e < k)) return 1;
    }
    return 0;
}
static f32 rs(int span) { return (f32)(rand() % (2 * span + 1) - span); }
int main(void) {
    long n = 0, bad = 0, hits = 0, i;
    static const f32 chk[] = { 0.0f, 1.0f, 5.0f, 20.0f, 50.0f };
    static const f32 det[] = { 0.0f, 300.0f };
    srand(12345);
    for (i = 0; i < 4000000; i++) {
        Vec3f v[3]; f32 z, x, ny; int k, span = (i & 1) ? 400 : 30;
        for (k = 0; k < 3; k++) { v[k].x = rs(span); v[k].y = rs(span); v[k].z = rs(span); }
        switch (i % 4) { /* points: anywhere near, on a vertex, on an edge, just off an edge */
            case 0: z = rs(span) + (rand() % 1000) / 1000.0f; x = rs(span) + (rand() % 1000) / 1000.0f; break;
            case 1: k = rand() % 3; z = v[k].z; x = v[k].x; break;
            default: { f32 t = (rand() % 1001) / 1000.0f; int a = rand() % 3, b = (a + 1) % 3;
                z = v[a].z + t * (v[b].z - v[a].z); x = v[a].x + t * (v[b].x - v[a].x);
                if (i % 4 == 3) { z += ((rand() % 21) - 10) * 0.1f; x += ((rand() % 21) - 10) * 0.1f; } }
        }
        ny = ((rand() % 2001) - 1000) / 1000.0f;
        for (k = 0; k < 10; k++) {
            f32 c = chk[k % 5], d = det[k / 5];
            s32 o = O_Impl(&v[0], &v[1], &v[2], z, x, d, c, ny);
            s32 f = Fishmod_TriChkPointParaY(&v[0], &v[1], &v[2], z, x, d, c, ny, 0);
            /* with the box test done by the caller: only where the box passes */
            if (O_Cir(v[0].z, v[0].x, v[1].z, v[1].x, v[2].z, v[2].x, z, x, c) && f != Fishmod_TriChkPointParaY(&v[0], &v[1], &v[2], z, x, d, c, ny, 1)) bad++;
            n++; hits += o; bad += (o != f);
        }
    }
    printf("%ld cases (%ld inside), %ld different\n", n, hits, bad);
    return 0;
}
