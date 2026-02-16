#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

// Public API for programs
void print(const char* str);
INTN fread(const char* path, void* buffer, UINTN max_size);
INTN fwrite(const char* path, const void* buffer, UINTN size);
void* alloc(UINTN size);
void free(void* ptr);
void exit();

#endif
