#ifndef UTILS_H
#define UTILS_H

#include "types.h"

static inline int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static inline void memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = (uint8*)dst;
    const uint8* s = (const uint8*)src;
    for (uint64 i = 0; i < n; i++) d[i] = s[i];
}

#endif
