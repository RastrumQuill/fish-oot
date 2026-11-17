#include "string.h"

char* strchr(const char* str, int c) {
    char ch = c;

    while (*str != ch) {
        if (*str == 0) {
            return NULL;
        }
        str++;
    }
    return (char*)str;
}

size_t strlen(const char* str) {
    const char* ptr = str;

    while (*ptr) {
        ptr++;
    }
    return ptr - str;
}

#ifndef FISHMOD_GCC // (fish-oot's GCC builds have their own, by words: src/gcc_fix/missing_gcc_functions.c)
void* memcpy(void* dst, const void* src, size_t n) {
    char* _dst = dst;
    const char* _src = src;

    while (n > 0) {
        *_dst++ = *_src++;
        n--;
    }
    return dst;
}
#endif
