#include <efi.h>
#include <efilib.h>
#include "memory.h"
#include "../include/system.h"

#define HEAP_SIZE (4 * 1024 * 1024) // 4MB heap

static uint8_t* heap_base = NULL;
static size_t heap_offset = 0;

void memory_init() {
    if (heap_base != NULL) return; // Already initialized

    EFI_STATUS status = ST->BootServices->AllocatePool(EfiLoaderData, HEAP_SIZE, (void**)&heap_base);
    if (EFI_ERROR(status)) {
        heap_base = NULL;
        return;
    }
    heap_offset = 0;
}

void* memory_alloc(size_t size) {
    if (heap_base == NULL || heap_offset + size > HEAP_SIZE) {
        return NULL;
    }
    void* ptr = heap_base + heap_offset;
    heap_offset += size;
    // Align to 16 bytes for general safety
    heap_offset = (heap_offset + 15) & ~15;
    return ptr;
}

void memory_free(void* ptr) {
    // Bump allocator doesn't support free
}

// Global API implementations
void* alloc(size_t size) {
    return memory_alloc(size);
}

void free(void* ptr) {
    memory_free(ptr);
}
