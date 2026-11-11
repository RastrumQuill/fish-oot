#ifndef FISHMOD_TRIG_H
#define FISHMOD_TRIG_H

#include "libc/math.h" /* sqrtf (an intrinsic: the sqrt.s instruction) */

/*
 * fish-oot: trig from short polynomials instead of tables and long series (Math_SinS/CosS, Math_SinF/CosF, sins/coss,
 * Math_Atan2S, Math_FAtanContFracF). Each is more accurate than what it replaces and needs no table (no cache misses
 * from a 2 KB or 4 KB table) and no long chain of divisions:
 * - sin/cos: by octants, a degree-3 polynomial for sin and a square root for cos (below): error 1.5e-4, against
 *   1.6e-3 for the original table (which steps every 16 binangs).
 * - atan on [0, 1]: an 8-term odd minimax polynomial: error 1.2e-7 rad. Math_Atan2S's table was off by up to 5.6
 *   binangs (now 0.5, the rounding); Math_FAtanContFracF's continued fraction (24 divisions a call, error 1.8e-7) is
 *   matched with none.
 * Checked against double precision on the PC (fishmod/tools/trig_check.c) before use.
 * Static functions, for IDO (libultra) as well as GCC: each file that includes this gets the ones it calls.
 */

#define FISHMOD_TRIG_BINANG_TO_RAD (6.28318530717958647692f / 65536.0f)

/*
 * sin and cos by octants (Kaze Emanuar's folded polynomial): three symmetries fold the angle into 0-45 degrees (the
 * half turn: the sign; the quarter: sin and cos swap; the octant: mirrored), a degree-3 polynomial gives sin there,
 * and cos comes from sqrt(1 - sin^2): a square root costs about what a division does on the VR4300, far less than a
 * second polynomial. Error 1.5e-4 (the table: 1.6e-3). For both of one angle (a rotation), Fishmod_SinCosBinang.
 */
#define FISHMOD_TRIG_OCT_SIN(t, t2) ((t) * (0.999031603f + (t2) * -0.160344586f))

/* the octant's angle in radians (0..pi/4), and whether sin and cos swap */
#define FISHMOD_TRIG_FOLD(a, oct, t)                       \
    {                                                      \
        s32 x_ = (a) & 0x1FFF;                             \
        (oct) = ((a) >> 13) & 7;                           \
        if ((oct) & 1) {                                   \
            x_ = 0x2000 - x_;                              \
        }                                                  \
        (t) = x_ * FISHMOD_TRIG_BINANG_TO_RAD;             \
    }

/* sin and cos of a binang (0x10000 a turn), together */
static void Fishmod_SinCosBinang(s16 angle, f32* sinOut, f32* cosOut) {
    s32 oct;
    f32 t;
    f32 s0;
    f32 c0;

    FISHMOD_TRIG_FOLD((u16)angle, oct, t);
    s0 = FISHMOD_TRIG_OCT_SIN(t, t * t);
    c0 = sqrtf(1.0f - s0 * s0);
    if (((oct + 1) >> 1) & 1) { /* octants 1, 2, 5, 6: swapped */
        f32 tmp = s0;

        s0 = c0;
        c0 = tmp;
    }
    /* (0.0f - x, not -x: a zero comes out +0 as the table gave) */
    *sinOut = (oct & 4) ? (0.0f - s0) : s0;              /* octants 4-7: below the axis */
    *cosOut = (((oct + 2) >> 2) & 1) ? (0.0f - c0) : c0; /* octants 2-5: behind it */
}

/* sin of a binang alone: the polynomial, and the square root only in the octants where sin is the swapped one */
static f32 Fishmod_SinBinang(s16 angle) {
    s32 oct;
    f32 t;
    f32 v;

    FISHMOD_TRIG_FOLD((u16)angle, oct, t);
    v = FISHMOD_TRIG_OCT_SIN(t, t * t);
    if (((oct + 1) >> 1) & 1) {
        v = sqrtf(1.0f - v * v);
    }
    return (oct & 4) ? (0.0f - v) : v;
}

/* cos of a binang: a quarter turn on from sin */
static f32 Fishmod_CosBinang(s16 angle) {
    return Fishmod_SinBinang((s16)(angle + 0x4000));
}

/* atan(t) for t in [0, 1], radians */
static f32 Fishmod_AtanUnit(f32 t) {
    f32 z = t * t;

    return t * (0.999999344f +
                z * (-0.333298594f +
                     z * (0.199465647f +
                          z * (-0.139086246f +
                               z * (0.0964218304f + z * (-0.0559121184f + z * (0.0218628068f + z * -0.00405452307f)))))));
}

#endif
