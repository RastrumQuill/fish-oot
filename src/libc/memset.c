#include "string.h"

/**
 * memset: sets `len` bytes to `val` starting at address `dest`.
 *
 * @see There are two other memsets in this codebase, Lib_MemSet(), MemSet()
 *
 * @param dest address to start at
 * @param val value to write (int, but interpreted as u8)
 * @param len number of bytes to write
 *
 * @return dest
 */
void* memset(void* dest, int val, size_t len) {
    unsigned char* ptr = dest;
    unsigned int word;

    // fishmod: a word at a time (the console profile found the byte loop at 2% of the CPU). Bytes until the address is
    // a word's, then 16 bytes a pass, then what's left.
    while ((len != 0) && (((unsigned int)ptr & 3) != 0)) {
        *ptr++ = val;
        len--;
    }
    word = (unsigned char)val;
    word |= word << 8;
    word |= word << 16;
    while (len >= 16) {
        ((unsigned int*)ptr)[0] = word;
        ((unsigned int*)ptr)[1] = word;
        ((unsigned int*)ptr)[2] = word;
        ((unsigned int*)ptr)[3] = word;
        ptr += 16;
        len -= 16;
    }
    while (len >= 4) {
        *(unsigned int*)ptr = word;
        ptr += 4;
        len -= 4;
    }
    while (len != 0) {
        *ptr++ = val;
        len--;
    }
    return dest;
}
