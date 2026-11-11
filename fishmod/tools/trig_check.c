/*
 * Checks fish-oot's polynomial trig (include/fishmod_trig.h) against double precision on the PC, with the originals
 * it replaces (libultra's sine table, Math_Atan2S's table, Math_FAtanContFracF's continued fraction) for comparison.
 * Every binang for sin/cos; a fine sweep for atan.
 *
 * Build and run (from the repository root, with the decomp's sources extracted):
 *   gcc -O2 -ffp-contract=off -D_LANGUAGE_C -Iinclude -o /tmp/trig_check fishmod/tools/trig_check.c -lm && /tmp/trig_check
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "ultra64/ultratypes.h"
#define M_PIf 3.14159265358979323846f

#include "../../include/fishmod_trig.h"
#include "../../src/libultra/gu/sintable.inc.c"

/* the originals */
static s16 Orig_sins(u16 angle) {
    s16 value;

    angle >>= 4;
    value = (angle & 0x400) ? sintable[0x3FF - (angle & 0x3FF)] : sintable[angle & 0x3FF];
    return (angle & 0x800) ? -value : value;
}

static f32 Orig_FAtanContFracF(f32 x) {
    s32 sector;
    f32 z, conv = 0.0f, sq;
    s32 i;

    if (x > 1.0f) {
        sector = 1;
        x = 1.0f / x;
    } else if (x < -1.0f) {
        sector = -1;
        x = 1.0f / x;
    } else {
        sector = 0;
    }
    sq = x * x;
    for (z = 24.0f, i = 24; i != 0; i--, z -= 1.0f) {
        conv = z * z * sq / (2.0f * z + 1.0f + conv);
    }
    if (sector > 0) {
        return M_PIf / 2 - (x / (1.0f + conv));
    } else if (sector < 0) {
        return -M_PIf / 2 - (x / (1.0f + conv));
    }
    return x / (1.0f + conv);
}

int main(void) {
    double maxOld = 0, maxNew = 0, maxOldC = 0, maxNewC = 0;
    s32 a;
    double maxAtanOld = 0, maxAtanNew = 0, maxTblOld = 0, maxTblNew = 0;
    s32 i;

    for (a = -32768; a < 32768; a++) {
        double r = a * (2.0 * M_PI / 65536.0);
        f32 s, c;

        Fishmod_SinCosBinang((s16)a, &s, &c);
        maxOld = fmax(maxOld, fabs(Orig_sins((u16)a) * (1.0 / 32767.0) - sin(r)));
        maxOldC = fmax(maxOldC, fabs(Orig_sins((u16)(a + 0x4000)) * (1.0 / 32767.0) - cos(r)));
        maxNew = fmax(maxNew, fabs(s - sin(r)));
        maxNewC = fmax(maxNewC, fabs(c - cos(r)));
    }
    printf("sin, every binang:  table %.2e   octant polynomial %.2e\n", maxOld, maxNew);
    printf("cos, every binang:  table %.2e   octant polynomial %.2e\n", maxOldC, maxNewC);
    {
        long differ = 0;

        for (a = -32768; a < 32768; a++) {
            f32 s, c;

            Fishmod_SinCosBinang((s16)a, &s, &c);
            differ += (s != Fishmod_SinBinang((s16)a)) + (c != Fishmod_CosBinang((s16)a));
        }
        printf("sin and cos alone against both together: %ld of 131072 values differ\n", differ);
    }
    {
        /* the cached Math_SinS/CosS: random sequences of calls (repeats, pairs, lone calls) against the plain ones */
        long differ = 0, calls;
        unsigned int x = 12345;
        s16 prev = 0;

        for (calls = 0; calls < 20000000; calls++) {
            s16 a2;
            f32 got, want;

            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            a2 = (x & 3) ? prev : (s16)(x >> 8); /* often the same angle again */
            if ((x >> 4) & 1) {
                got = Fishmod_SinCached(a2);
                want = Fishmod_SinBinang(a2);
            } else {
                got = Fishmod_CosCached(a2);
                want = Fishmod_CosBinang(a2);
            }
            differ += (memcmp(&got, &want, sizeof(f32)) != 0); /* (bit for bit: u32 is 8 bytes on the PC) */
            prev = a2;
        }
        printf("cached sin/cos against plain, 20,000,000 calls in random order: %ld differ\n", differ);
    }

    for (i = -2000000; i <= 2000000; i++) {
        f32 x = i * 0.00005f; /* -100..100 */
        double ref = atan((double)x);
        f32 ax = fabsf(x), t, v;

        maxAtanOld = fmax(maxAtanOld, fabs(Orig_FAtanContFracF(x) - ref));
        t = (ax > 1.0f) ? 1.0f / ax : ax;
        v = Fishmod_AtanUnit(t);
        if (ax > 1.0f) {
            v = M_PIf / 2 - v;
        }
        if (x < 0.0f) {
            v = -v;
        }
        maxAtanNew = fmax(maxAtanNew, fabs(v - ref));
    }
    printf("atan, -100..100:    continued fraction %.2e   polynomial %.2e (rad)\n", maxAtanOld, maxAtanNew);

    /* Math_Atan2S's table: index round(t * 1024), in binang */
    for (i = 0; i <= 1000000; i++) {
        f32 t = i / 1000000.0f;
        double ref = atan((double)t) * (32768.0 / M_PI);
        double tbl = round(atan(floor(t * 1024.0f + 0.5f) / 1024.0) * (32768.0 / M_PI)); /* the table's own entry */
        f32 v = Fishmod_AtanUnit(t) * (32768.0f / M_PIf);

        maxTblOld = fmax(maxTblOld, fabs(tbl - ref));
        maxTblNew = fmax(maxTblNew, fabs(floorf(v + 0.5f) - ref));
    }
    printf("atan2 (binang):     table %.2f   polynomial, rounded %.2f\n", maxTblOld, maxTblNew);
    return 0;
}
