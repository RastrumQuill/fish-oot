#ifndef FISHMOD_TRIG_H
#define FISHMOD_TRIG_H

/*
 * fish-oot: trig from short polynomials instead of tables and long series (Math_SinS/CosS, Math_SinF/CosF, sins/coss,
 * Math_Atan2S, Math_FAtanContFracF). Each is more accurate than what it replaces and needs no table (no cache misses
 * from a 2 KB or 4 KB table) and no long chain of divisions:
 * - sin/cos: the angle (a binang) folded to within 45 degrees of the nearest quarter turn, then the minimax
 *   polynomials for that range (Cephes' sinf/cosf): error 1e-7, against 1.6e-3 for the original table (which steps
 *   every 16 binangs).
 * - atan on [0, 1]: an 8-term odd minimax polynomial: error 1.2e-7 rad. Math_Atan2S's table was off by up to 5.6
 *   binangs (now 0.5, the rounding); Math_FAtanContFracF's continued fraction (24 divisions a call, error 1.8e-7) is
 *   matched with none.
 * Checked against double precision on the PC (fishmod/tools/trig_check.c) before use.
 * Static functions, for IDO (libultra) as well as GCC: each file that includes this gets the ones it calls.
 */

#define FISHMOD_TRIG_BINANG_TO_RAD (6.28318530717958647692f / 65536.0f)

/* sin and cos of a binang (0x10000 a turn) */
static void Fishmod_SinCosBinang(s16 angle, f32* sinOut, f32* cosOut) {
    s32 quarter = (((s32)angle + 0x2000) >> 14) & 3;
    f32 t = (s16)(angle - (quarter << 14)) * FISHMOD_TRIG_BINANG_TO_RAD; /* within +-pi/4 */
    f32 t2 = t * t;
    f32 s = t + t * t2 * (-1.6666654611e-1f + t2 * (8.3321608736e-3f + t2 * -1.9515295891e-4f));
    f32 c = 1.0f - 0.5f * t2 + t2 * t2 * (4.166664568298827e-2f + t2 * (-1.388731625493765e-3f + t2 * 2.443315711809948e-5f));

    /* (0.0f - x, not -x: a zero comes out +0 as the table gave) */
    switch (quarter) {
        case 0:
            *sinOut = s;
            *cosOut = c;
            break;
        case 1:
            *sinOut = c;
            *cosOut = 0.0f - s;
            break;
        case 2:
            *sinOut = 0.0f - s;
            *cosOut = 0.0f - c;
            break;
        default:
            *sinOut = 0.0f - c;
            *cosOut = s;
            break;
    }
}

static f32 Fishmod_SinBinang(s16 angle) {
    f32 s;
    f32 c;

    Fishmod_SinCosBinang(angle, &s, &c);
    return s;
}

static f32 Fishmod_CosBinang(s16 angle) {
    f32 s;
    f32 c;

    Fishmod_SinCosBinang(angle, &s, &c);
    return c;
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
