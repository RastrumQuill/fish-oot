#include "string.h"
#include "ultra64/ultratypes.h"

/**
 * memmove: copies `len` bytes from memory starting at `src` to memory starting at `dest`.
 *
 * Unlike memcpy(), the regions of memory may overlap.
 *
 * @param dest address of start of buffer to write to
 * @param src address of start of buffer to read from
 * @param len number of bytes to copy.
 *
 * @return dest
 */
void* memmove(void* dest, const void* src, size_t len) {
    char* d = dest;
    const char* s = src;

    if (d == s) {
        return dest;
    }
    // fish-oot: four bytes at a time where both are on the same footing (bcopy comes here in GCC builds, and went byte
    // by byte: a fish's 160-byte tint table took about 48 us in ares). The same bytes end up where they did, overlap
    // or not: forward when the destination is below the source, backward when above, as before.
    if ((((u32)d ^ (u32)s) & 3) == 0) {
        if (d < s) {
            while ((len != 0) && ((u32)d & 3)) {
                *d++ = *s++;
                len--;
            }
            while (len >= 16) {
                u32 a = ((const u32*)s)[0];
                u32 b = ((const u32*)s)[1];
                u32 c = ((const u32*)s)[2];
                u32 e = ((const u32*)s)[3];

                ((u32*)d)[0] = a;
                ((u32*)d)[1] = b;
                ((u32*)d)[2] = c;
                ((u32*)d)[3] = e;
                d += 16;
                s += 16;
                len -= 16;
            }
            while (len >= 4) {
                *(u32*)d = *(const u32*)s;
                d += 4;
                s += 4;
                len -= 4;
            }
            while (len--) {
                *d++ = *s++;
            }
        } else {
            d += len;
            s += len;
            while ((len != 0) && ((u32)d & 3)) {
                *--d = *--s;
                len--;
            }
            while (len >= 16) {
                u32 a;
                u32 b;
                u32 c;
                u32 e;

                d -= 16;
                s -= 16;
                a = ((const u32*)s)[3];
                b = ((const u32*)s)[2];
                c = ((const u32*)s)[1];
                e = ((const u32*)s)[0];
                ((u32*)d)[3] = a;
                ((u32*)d)[2] = b;
                ((u32*)d)[1] = c;
                ((u32*)d)[0] = e;
                len -= 16;
            }
            while (len >= 4) {
                d -= 4;
                s -= 4;
                *(u32*)d = *(const u32*)s;
                len -= 4;
            }
            while (len--) {
                *--d = *--s;
            }
        }
        return dest;
    }
    if (d < s) {
        while (len--) {
            *d++ = *s++;
        }
    } else {
        d += len - 1;
        s += len - 1;
        while (len--) {
            *d-- = *s--;
        }
    }
    return dest;
}
