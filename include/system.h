#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

/* Public API for programs - Category 12: Use custom types */
void print(const char* str);
INTN fread(const char* path, void* buffer, uint64 max_size);
INTN fwrite(const char* path, const void* buffer, uint64 size);
void* alloc(uint64 size);
void free(void* ptr);
void exit();

#endif
