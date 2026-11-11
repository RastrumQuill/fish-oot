#include "ultra64.h"

#include "fishmod_trig.h"

/**
 * @param angle binang
 * @return sin(angle)*0x7FFF
 */
s16 sins(u16 angle) {
    // fish-oot: from the polynomial (fishmod_trig.h), rounded, instead of the table (which steps every 16 binangs)
    f32 s = Fishmod_SinBinang((s16)angle) * 32767.0f;

    return (s16)((s >= 0.0f) ? (s + 0.5f) : (s - 0.5f));
}
