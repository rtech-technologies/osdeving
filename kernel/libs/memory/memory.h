#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

void memory_init();
void memory_get_usage(size_t* total, size_t* used);
void* memory_alloc(UINTN size);
void memory_free(void* ptr);

// Platform interface for the bootloader to provide the heap
void memory_set_heap(void* base, UINTN size);

// Memory self-test
int memory_self_test();

#endif
