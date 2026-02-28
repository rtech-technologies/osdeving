#ifndef MEMORY_H
#define MEMORY_H

#include "../include/types.h"

void memory_init();
void* alloc(uint64 size);
void free(void* ptr);

#endif
