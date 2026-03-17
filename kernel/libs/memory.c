#include "memory.h"
#include "../unice64/kernel.h"

static uint64 heap_ptr = 0;

typedef struct free_block {
    uint32 size;
    struct free_block* next;
} free_block_t;

static free_block_t* free_list = (void*)0;

void memory_init() {
    heap_ptr = 0;
    free_list = (void*)0;
}

void* alloc(uint64 size) {
    if (!kboot_params.heap_base) {
        return (void*)0;
    }

    uint64 total_size = sizeof(arc_header_t) + size;
    total_size = (total_size + 15) & ~15;

    /* Check free list first */
    free_block_t** curr = &free_list;
    while (*curr) {
        if ((*curr)->size >= total_size) {
            free_block_t* found = *curr;
            *curr = found->next;

            arc_header_t* header = (arc_header_t*)found;
            header->ref_count = 1;
            header->size = (uint32)size;
            header->magic = ARC_MAGIC;
            return (void*)(header + 1);
        }
        curr = &((*curr)->next);
    }

    /* Use configurable heap size limit from config.h */
    uint64 max_heap = (uint64)CONFIG_HEAP_SIZE_MB * 1024 * 1024;
    uint64 effective_limit = (kboot_params.heap_size < max_heap && kboot_params.heap_size > 0) ? kboot_params.heap_size : max_heap;

    if (heap_ptr + total_size > effective_limit) {
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
                /* Actual Reclamation */
                uint32 total_size = (sizeof(arc_header_t) + header->size + 15) & ~15;
                header->magic = 0;

                free_block_t* free_b = (free_block_t*)header;
                free_b->size = total_size;
                free_b->next = free_list;
                free_list = free_b;
            }
        }
    }
}
