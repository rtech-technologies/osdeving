#include "memory.h"
#include "../unice64/kernel.h"

static uint64 heap_ptr = 0;

void memory_init() {
    heap_ptr = 0;
}

void* alloc(uint64 size) {
    if (!kboot_params.heap_base) {
        return (void*)0;
    }

    /* Total size = ARC header + payload + alignment */
    uint64 total_size = sizeof(arc_header_t) + size;
    total_size = (total_size + 15) & ~15;

    if (heap_ptr + total_size > kboot_params.heap_size) {
        return (void*)0;
    }

    arc_header_t* header = (arc_header_t*)((uint8*)kboot_params.heap_base + heap_ptr);
    header->ref_count = 1;
    header->size = (uint32)size;
    header->magic = ARC_MAGIC;

    heap_ptr += total_size;

    return (void*)(header + 1);
}

void retain(void* ptr) {
    if (!ptr) {
        return;
    }
    arc_header_t* header = (arc_header_t*)ptr - 1;
    if (header->magic == ARC_MAGIC) {
        header->ref_count++;
    }
}

void release(void* ptr) {
    if (!ptr) {
        return;
    }
    arc_header_t* header = (arc_header_t*)ptr - 1;
    if (header->magic == ARC_MAGIC) {
        if (header->ref_count > 0) {
            header->ref_count--;
            if (header->ref_count == 0) {
                /* In this simple version, we don't reclaim memory,
                   but we invalidate the magic. */
                header->magic = 0;
            }
        }
    }
}
