#include "memory.h"
#include "../kernel/kernel.h"

static uint64 heap_ptr = 0;

void memory_init() {
    heap_ptr = 0;
}

void* alloc(uint64 size) {
    /* Align to 16 bytes */
    size = (size + 15) & ~15;

    if (!kboot_params.heap_base) return NULL;
    if (heap_ptr + size > kboot_params.heap_size) return NULL;

    void* ptr = (uint8*)kboot_params.heap_base + heap_ptr;
    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    /* Bump allocator - no free */
}
