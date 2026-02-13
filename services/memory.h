#ifndef MEMORY_H
#define MEMORY_H

#include "../include/types.h"

void memory_init();
void* memory_alloc(size_t size);
void memory_free(void* ptr);

#endif
