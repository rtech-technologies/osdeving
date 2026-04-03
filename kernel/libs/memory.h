#ifndef MEMORY_H
#define MEMORY_H

#include "../../include/types.h"

/* ARC Header for Python-like memory management */
typedef struct {
    uint32 ref_count;
    uint32 size;
    uint64 magic; /* To verify ARC pointer */
} arc_header_t;

#define ARC_MAGIC 0x4152434D454D3031 /* "ARCMEM01" */

void memory_init();
void* alloc(uint64 size);
void retain(void* ptr);
void release(void* ptr);

#endif
