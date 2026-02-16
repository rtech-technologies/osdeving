#include "memory.h"

#define HEAP_SIZE (1024 * 1024)
static uint8 global_heap[HEAP_SIZE];
static UINTN heap_ptr = 0;

void memory_init() {
    heap_ptr = 0;
}

void* alloc(UINTN size) {
    if (heap_ptr + size > HEAP_SIZE) return NULL;
    void* ptr = &global_heap[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    // Bump allocator doesn't support free
}
