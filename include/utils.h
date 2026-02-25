#ifndef UTILS_H
#define UTILS_H

#include "types.h"

static inline int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static inline void memcpy(void* dst, const void* src, uint64 n) {
    if (!dst || !src || n == 0) return;
    uint8* d = (uint8*)dst;
    const uint8* s = (const uint8*)src;
    for (uint64 i = 0; i < n; i++) d[i] = s[i];
}

static inline void memset(void* dst, uint8 val, uint64 n) {
    if (!dst || n == 0) return;
    uint8* d = (uint8*)dst;
    for (uint64 i = 0; i < n; i++) d[i] = val;
}

static inline void char_strncpy(char* dst, const char* src, uint64 max_len) {
    if (!dst || !src || max_len == 0) return;
    uint64 i = 0;
    while (i < max_len - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

#endif
