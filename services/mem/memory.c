#include "memory.h"
#include "../../include/types.h"

#define HEAP_SIZE (4 * 1024 * 1024)
static uint8 global_heap[HEAP_SIZE];
static uint64 heap_ptr = 0;

void memory_init() {
    heap_ptr = 0;
}

void* alloc(uint64 size) {
    /* Align to 16 bytes */
    size = (size + 15) & ~15;

    if (heap_ptr + size > HEAP_SIZE) return NULL;

    void* ptr = &global_heap[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    /* Bump allocator - no free */
}
