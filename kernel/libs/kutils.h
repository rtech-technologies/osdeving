#ifndef KUTILS_H
#define KUTILS_H

#include "../../include/types.h"

int  strcmp(const char* s1, const char* s2);
void strcpy(char* dst, const char* src);
void memcpy(void* dst, const void* src, uint64 n);
void memset(void* s, int c, uint64 n);
void itoa(int n, char* s, int base);
uint32 crc32(const void* data, uint64 len);

#endif
