#ifndef RNAFS_H
#define RNAFS_H

#include "../include/types.h"

typedef struct {
    char name[64];
    uint64 offset;
    uint64 size;
    uint64 flags;
} rnafs_entry_t;

#define RNAFS_MAX_FILES 64

void rnafs_init();
INTN rnafs_read(const char* path, void* buffer, uint64 max_size);
INTN rnafs_write(const char* path, const void* buffer, uint64 size);

#endif
