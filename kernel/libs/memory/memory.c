#include "memory.h"
#include "system.h"

static uint8* heap_base = NULL;
static UINTN  heap_size = 0;
static UINTN  heap_offset = 0;

void memory_set_heap(void* base, size_t size) {
    heap_base = (uint8*)base;
    heap_size = (UINTN)size;
    heap_offset = 0;
}

void memory_init() {
}

void* memory_alloc(size_t size) {
    if (heap_base == NULL || heap_offset + (UINTN)size > heap_size) {
        return NULL;
    }
    void* ptr = (void*)(heap_base + heap_offset);
    heap_offset += (UINTN)size;
    // Align to 16 bytes
    heap_offset = (heap_offset + 15) & ~15;
    return ptr;
}

void memory_free(void* ptr) {
}

int memory_self_test() {
    // Disabled for stability in v0
    return 0;
}

// Global API
void* alloc(size_t size) {
    return memory_alloc(size);
}

void free(void* ptr) {
    memory_free(ptr);
}
