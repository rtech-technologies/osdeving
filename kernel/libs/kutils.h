#ifndef KUTILS_H
#define KUTILS_H

#include "../../include/types.h"

int strcmp(const char* s1, const char* s2);
void strcpy(char* dst, const char* src);
void k_strcat(char* dst, const char* src);
uint64 k_strlen(const char* s);
void k_memcpy(void* dst, const void* src, uint64 n);
void k_memset(void* s, int c, uint64 n);
void itoa(int n, char* s, int base);
uint32 crc32(const void* data, uint64 len);

void serial_init();
void serial_print(const char* str);

/* Alias for kernel use if needed, but we'll use k_ prefix to avoid conflicts with gnu-efi */
#define memcpy k_memcpy
#define memset k_memset
#define strlen k_strlen
#define strcat k_strcat

#endif
