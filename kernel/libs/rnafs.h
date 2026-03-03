#ifndef RNAFS_H
#define RNAFS_H

#include "../../include/types.h"

/* RNAFS v1 Structure */
#define RNAFS_MAGIC 0x5346414E52 /* "RNAFS" */
#define RNAFS_BLOCK_SIZE 512

typedef struct {
    uint64 magic;
    uint64 total_blocks;
    uint64 bitmap_start;
    uint64 bitmap_blocks;
    uint64 dir_start;
    uint64 dir_blocks;
    uint64 data_start;
} rnafs_superblock_t;

typedef struct {
    char name[64];
    uint64 start_block;
    uint64 size;
    uint32 flags;
    uint32 padding;
} rnafs_entry_t;

#define RNAFS_MAX_FILES 64

void rnafs_init();
INTN rnafs_read(const char* path, void* buffer, uint64 max_size);
INTN rnafs_write(const char* path, const void* buffer, uint64 size);

#endif
