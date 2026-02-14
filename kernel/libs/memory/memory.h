#ifndef MEMORY_H
#define MEMORY_H

#include "../../../include/types.h"

void memory_init();
void* memory_alloc(UINTN size);
void memory_free(void* ptr);

// Platform interface for the bootloader to provide the heap
void memory_set_heap(void* base, UINTN size);

// Memory self-test
int memory_self_test();

#endif
