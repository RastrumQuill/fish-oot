/* --------------------------------------------------------------------------------*/
/* Depending on the toolchain used, an appropriate precompiled libgcc library      */
/* may not exist and cannot be linked against. Until we have a better work around, */
/* necessary gcc functions are hosted here in order to properly compile.           */
/* This file is NOT a part of the original game and only exists to help gcc work.  */
/* --------------------------------------------------------------------------------*/

#include "ultra64.h"

// Self-hosted libc memory functions, gcc assumes these exist even in a freestanding
// environment and there is no way to tell it otherwise.

int memcmp(const void* s1, const void* s2, size_t n) {
    const u8* m1 = s1;
    const u8* m2 = s2;
    size_t i;

    for (i = 0; i < n; i++) {
        if (m1[i] < m2[i]) {
            return -1;
        } else if (m1[i] > m2[i]) {
            return 1;
        }
    }

    return 0;
}

// Conversions involving 64-bit integer types required by the O32 MIPS ABI.

// f32 -> u64, negative values become 0
u64 __fixunssfdi(f32 a) {
    if (a > 0.0f) {
        register union {
            f64 f;
            u64 i;
        } m;

        __asm__("cvt.l.s %0, %1" : "=f"(m.f) : "f"(a));
        return m.i;
    }
    return 0;
}

// f64 -> u64, negative values become 0
u64 __fixunsdfdi(f64 a) {
    if (a > 0.0) {
        register union {
            f64 f;
            u64 i;
        } m;

        __asm__("cvt.l.d %0, %1" : "=f"(m.f) : "f"(a));
        return m.i;
    }
    return 0;
}

// f32 -> s64
s64 __fixsfdi(f32 c) {
    register union {
        f64 f;
        s64 i;
    } m;

    __asm__("cvt.l.s %0, %1" : "=f"(m.f) : "f"(c));
    return m.i;
}

// f64 -> s64
s64 __fixdfdi(f64 c) {
    register union {
        f64 f;
        s64 i;
    } m;

    __asm__("cvt.l.d %0, %1" : "=f"(m.f) : "f"(c));
    return m.i;
}

// s64 -> f32
f32 __floatdisf(s64 c) {
    register union {
        f64 f;
        s64 i;
    } m;
    register f32 v;

    m.i = c;
    __asm__("cvt.s.l %0, %1" : "=f"(v) : "f"(m.f));
    return v;
}

// s64 -> f64
f64 __floatdidf(s64 c) {
    register union {
        f64 f;
        s64 i;
    } m;
    register f64 v;

    m.i = c;
    __asm__("cvt.d.l %0, %1" : "=f"(v) : "f"(m.f));
    return v;
}

// u64 -> f32
f32 __floatundisf(u64 c) {
    register union {
        f64 f;
        u64 i;
    } m;
    register f32 v;

    m.i = c;
    __asm__("cvt.s.l %0, %1" : "=f"(v) : "f"(m.f));
    if ((s64)c < 0) {
        // cvt.s.l assumes signed input, adjust output
        v += 4294967296.0f; // 2^32
    }
    return v;
}

// u64 -> f64
f64 __floatundidf(u64 c) {
    register union {
        f64 f;
        u64 i;
    } m;
    register f64 v;

    m.i = c;
    __asm__("cvt.d.l %0, %1" : "=f"(v) : "f"(m.f));
    if ((s64)c < 0) {
        // cvt.d.l assumes signed input, adjust output
        v += 18446744073709551616.0; // 2^64
    }
    return v;
}

// Compute x^m by binary exponentiation

f32 __powisf2(f32 x, s32 m) {
    u32 n = (m < 0) ? -m : m;
    f32 y = (n % 2 != 0) ? x : 1.0f;

    while (n >>= 1) {
        x = x * x;

        if (n % 2 != 0) {
            y = y * x;
        }
    }
    return (m < 0) ? (1.0f / y) : y;
}

// Compute division and modulo of 64-bit signed and unsigned integers

__asm__("                                   \n\
    .set push                               \n\
    .set noreorder                          \n\
    .set gp=64                              \n\
                                            \n\
.global __umoddi3                           \n\
__umoddi3:                                  \n\
  .type __umoddi3, @function                \n\
  .ent __umoddi3                            \n\
    sw      $a0, 0x0($sp)                   \n\
    sw      $a1, 0x4($sp)                   \n\
    sw      $a2, 0x8($sp)                   \n\
    sw      $a3, 0xC($sp)                   \n\
    ld      $t6, 0($sp)                     \n\
    ld      $t7, 8($sp)                     \n\
    dremu   $v0, $t6, $t7                   \n\
    dsll32  $v1, $v0, 0                     \n\
    dsra32  $v1, $v1, 0                     \n\
    jr      $ra                             \n\
     dsra32 $v0, $v0, 0                     \n\
  .end __umoddi3                            \n\
  .size __umoddi3, . - __umoddi3            \n\
                                            \n\
.global __udivdi3                           \n\
__udivdi3:                                  \n\
  .type __udivdi3, @function                \n\
  .ent __udivdi3                            \n\
    sw      $a0, 0x0($sp)                   \n\
    sw      $a1, 0x4($sp)                   \n\
    sw      $a2, 0x8($sp)                   \n\
    sw      $a3, 0xC($sp)                   \n\
    ld      $t6, 0($sp)                     \n\
    ld      $t7, 8($sp)                     \n\
    ddivu   $v0, $t6, $t7                   \n\
    dsll32  $v1, $v0, 0                     \n\
    dsra32  $v1, $v1, 0                     \n\
    jr      $ra                             \n\
     dsra32 $v0, $v0, 0                     \n\
  .end __udivdi3                            \n\
  .size __udivdi3, . - __udivdi3            \n\
                                            \n\
.global __moddi3                            \n\
__moddi3:                                   \n\
  .type __moddi3, @function                 \n\
  .ent __moddi3                             \n\
    sw      $a0, 0x0($sp)                   \n\
    sw      $a1, 0x4($sp)                   \n\
    sw      $a2, 0x8($sp)                   \n\
    sw      $a3, 0xC($sp)                   \n\
    ld      $t6, 0($sp)                     \n\
    ld      $t7, 8($sp)                     \n\
    drem    $v0, $t6, $t7                   \n\
    dsll32  $v1, $v0, 0                     \n\
    dsra32  $v1, $v1, 0                     \n\
    jr      $ra                             \n\
     dsra32 $v0, $v0, 0                     \n\
  .end __moddi3                             \n\
  .size __moddi3, . - __moddi3              \n\
                                            \n\
.global __divdi3                            \n\
__divdi3:                                   \n\
  .type __divdi3, @function                 \n\
  .ent __divdi3                             \n\
    sw    $a0, 0x0($sp)                     \n\
    sw    $a1, 0x4($sp)                     \n\
    sw    $a2, 0x8($sp)                     \n\
    sw    $a3, 0xC($sp)                     \n\
    ld    $t6, 0($sp)                       \n\
    ld    $t7, 8($sp)                       \n\
    ddiv  $v0, $t6, $t7                     \n\
    dsll32 $v1, $v0, 0                      \n\
    dsra32 $v1, $v1, 0                      \n\
    jr    $ra                               \n\
     dsra32 $v0, $v0, 0                     \n\
  .end __divdi3                             \n\
  .size __divdi3, . - __divdi3              \n\
                                            \n\
    .set pop                                \n\
                                            \n");

#ifdef FISHMOD_GCC
// fish-oot: memcpy by words. Compiled for size, GCC calls memcpy for every structure it copies (a matrix, a vector);
// libultra's own copies a byte at a time, and was 4.7% of the CPU's working time at Lake Hylia in the first -Os
// build. Aligned blocks (nearly all of them: structures) go four words, then one word, at a time.
void* memcpy(void* dst, const void* src, size_t n) {
    u32* d = dst;
    const u32* s = src;
    u8* db;
    const u8* sb;

    if ((((u32)dst | (u32)src) & 3) == 0) {
        while (n >= 16) {
            u32 a = s[0];
            u32 b = s[1];
            u32 c = s[2];
            u32 e = s[3];

            d[0] = a;
            d[1] = b;
            d[2] = c;
            d[3] = e;
            d += 4;
            s += 4;
            n -= 16;
        }
        while (n >= 4) {
            *d++ = *s++;
            n -= 4;
        }
    }
    db = (u8*)d;
    sb = (const u8*)s;
    while (n != 0) {
        *db++ = *sb++;
        n--;
    }
    return dst;
}

// fish-oot: the helpers its GCC builds call beyond the above. 64-bit shifts, on the two 32-bit halves (written with
// 32-bit operations only: a 64-bit shift here would be a call to itself)
long long __ashldi3(long long a, int b) {
    unsigned int lo = (unsigned int)a;
    unsigned int hi = (unsigned int)((unsigned long long)a >> 32);

    if (b >= 32) {
        hi = lo << (b - 32);
        lo = 0;
    } else if (b > 0) {
        hi = (hi << b) | (lo >> (32 - b));
        lo <<= b;
    }
    return (long long)(((unsigned long long)hi << 32) | lo);
}
unsigned long long __lshrdi3(unsigned long long a, int b) {
    unsigned int lo = (unsigned int)a;
    unsigned int hi = (unsigned int)(a >> 32);

    if (b >= 32) {
        lo = hi >> (b - 32);
        hi = 0;
    } else if (b > 0) {
        lo = (lo >> b) | (hi << (32 - b));
        hi >>= b;
    }
    return ((unsigned long long)hi << 32) | lo;
}
long long __ashrdi3(long long a, int b) {
    unsigned int lo = (unsigned int)a;
    int hi = (int)((unsigned long long)a >> 32);

    if (b >= 32) {
        lo = (unsigned int)(hi >> (b - 32));
        hi >>= 31;
    } else if (b > 0) {
        lo = (lo >> b) | ((unsigned int)hi << (32 - b));
        hi >>= b;
    }
    return (long long)(((unsigned long long)(unsigned int)hi << 32) | lo);
}
// GCC -Os turns some string formatting into a call to strcpy, which the game's libc doesn't have
char* strcpy(char* dst, const char* src) {
    char* d = dst;

    while ((*d++ = *src++) != '\0') {}
    return dst;
}
#endif
