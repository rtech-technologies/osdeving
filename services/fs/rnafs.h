#ifndef RNAFS_H
#define RNAFS_H

#include "../../include/types.h"

#define FS_BLOCK_SIZE 4096
#define SECTOR_SIZE   512
#define SECTORS_PER_BLOCK (FS_BLOCK_SIZE / SECTOR_SIZE)

/* Category 3: Packed structs */
typedef struct __attribute__((packed)) {
    uint8  magic[6];
    uint16 reserved;
    uint32 block_size;
    uint32 total_blocks;
    uint32 bitmap_start_block;
    uint32 bitmap_block_count;
    uint32 dir_start_block;
    uint32 dir_block_count;
    uint32 data_start_block;
    uint64 partition_start_lba;
} Superblock;

typedef struct __attribute__((packed)) {
    char   name[32];
    uint32 type;         /* 0: free, 1: file, 2: dir */
    uint32 first_block;
    uint32 block_count;
    uint32 size_bytes;
    uint32 parent_index;
    uint8  reserved[12];
} DirEntry;

/* Category 12: Custom types and naming */
int rnafs_mount(uint64 partition_start_lba);
int rnafs_create_file(uint32 parent, const char* name, uint32 size_bytes);
int rnafs_read_file(const char* name, void* buffer, uint32 max_bytes);
int rnafs_write_file(const char* name, const void* buffer, uint32 size_bytes);
void rnafs_debug_dump_superblock(uint64 lba);

#endif
