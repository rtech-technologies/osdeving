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

typedef struct __attribute__((packed)) {
    char name[64];
    uint64 start_block;
    uint64 size;
    uint32 flags;
    uint8 padding[44];
} rnafs_entry_t;

#define RNAFS_MAX_FILES 16

void rnafs_init();
void rnafs_format_vdisk(const char* vdisk_name, uint32 size_sectors);
void rnafs_mount_vdisk(const char* vdisk_name);
void rnafs_ls();
INTN rnafs_read(const char* path, void* buffer, uint64 max_size);
INTN rnafs_write(const char* path, const void* buffer, uint64 size);

#endif
