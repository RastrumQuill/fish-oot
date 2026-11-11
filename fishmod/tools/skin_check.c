/* fish-oot: SkinMatrix's translate-rotate(-scale) matrices written directly (src/code/z_skin_matrix.c) against the
   original products, for random rotations, scales and positions: every entry must compare equal (== on floats: a
   zero's sign is the only difference the products can make). cc -O2 -o /tmp/skin_check fishmod/tools/skin_check.c -lm */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef float f32;
typedef struct { f32 xx, yx, zx, wx, xy, yy, zy, wy, xz, yz, zz, wz, xw, yw, zw, ww; } MtxF;

static void Mult(MtxF* a, MtxF* b, MtxF* d) { /* SkinMatrix_MtxFMtxFMult's expressions */
    f32 r[4][4], A[4][4], B[4][4];
    f32* pa = &a->xx; f32* pb = &b->xx;
    int i, j;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) { A[i][j] = pa[j * 4 + i]; B[i][j] = pb[j * 4 + i]; }
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
        r[i][j] = (A[i][0] * B[0][j]) + (A[i][1] * B[1][j]) + (A[i][2] * B[2][j]) + (A[i][3] * B[3][j]);
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) (&d->xx)[j * 4 + i] = r[i][j];
}
static f32 rnd(f32 lo, f32 hi) { return lo + (hi - lo) * (rand() / (f32)RAND_MAX); }
static void Rot(MtxF* m) { /* any rotation-shaped matrix: 3x3 values, the rest of the identity */
    f32 a = rnd(-7, 7), b = rnd(-7, 7), c = rnd(-7, 7);
    m->xx = cosf(a) * cosf(b); m->yx = sinf(a) * cosf(b); m->zx = -sinf(b); m->wx = 0;
    m->xy = sinf(c) * (rand() & 1 ? 0.0f : -0.5f); m->yy = cosf(c); m->zy = sinf(a); m->wy = 0;
    m->xz = -sinf(c); m->yz = (rand() & 3) ? cosf(a) * sinf(c) : 0.0f; m->zz = cosf(b) * cosf(c); m->wz = 0;
    m->xw = m->yw = m->zw = 0; m->ww = 1;
}
int main(void) {
    long n, bad = 0;
    for (n = 0; n < 5000000; n++) {
        MtxF r, t, s, tmp, old, neu;
        f32 sx = rnd(-3, 3), sy = (n & 7) ? rnd(-3, 3) : 0.0f, sz = rnd(-3, 3);
        f32 tx = rnd(-30000, 30000), ty = (n & 15) ? rnd(-30000, 30000) : -0.0f, tz = rnd(-30000, 30000);
        int i;
        Rot(&r);
        t = (MtxF){ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, tx, ty, tz, 1 };
        s = (MtxF){ sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1 };
        Mult(&t, &r, &tmp);
        Mult(&tmp, &s, &old);
        neu = r;
        neu.xx *= sx; neu.yx *= sx; neu.zx *= sx; neu.xy *= sy; neu.yy *= sy; neu.zy *= sy;
        neu.xz *= sz; neu.yz *= sz; neu.zz *= sz; neu.xw = tx; neu.yw = ty; neu.zw = tz;
        for (i = 0; i < 16; i++) if ((&old.xx)[i] != (&neu.xx)[i]) { bad++; break; }
        /* translate-rotate alone */
        neu = r; neu.xw = tx; neu.yw = ty; neu.zw = tz;
        for (i = 0; i < 16; i++) if ((&tmp.xx)[i] != (&neu.xx)[i]) { bad++; break; }
    }
    printf("%ld cases, %ld different\n", n * 2, bad);
    return bad != 0;
}
