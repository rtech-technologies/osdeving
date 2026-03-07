#include "../../include/types.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

void memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = (uint8*)dst;
    const uint8* s = (const uint8*)src;
    while (n--) *d++ = *s++;
}

void memset(void* s, int c, uint64 n) {
    uint8* p = (uint8*)s;
    while (n--) *p++ = (uint8)c;
}

void itoa(int n, char* s, int base) {
    char* p = s;
    char* p1, *p2;
    unsigned int ud = n;
    if (base == 10 && n < 0) {
        *p++ = '-';
        s++;
        ud = -n;
    }
    do {
        int remainder = ud % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= base);
    *p = 0;
    p1 = s;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
}
