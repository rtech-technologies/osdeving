#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

void print(const char* str);
void* alloc(size_t size);
void free(void* ptr);
int fread(const char* path, void* buffer);
int fwrite(const char* path, const void* buffer);
void exit();

#endif
