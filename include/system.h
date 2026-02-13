#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

void print(const char* str);
void* alloc(size_t size);
void free(void* ptr);
INTN fread(const char* path, void* buffer, UINTN max_size);
INTN fwrite(const char* path, const void* buffer);
void wait_for_key();
char read_key();
void exit();

#endif
