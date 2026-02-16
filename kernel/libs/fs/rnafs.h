#ifndef RNAFS_H
#define RNAFS_H

#include "types.h"

#define FS_BLOCK_SIZE 4096
#define SECTOR_SIZE   512
#define SECTORS_PER_BLOCK (FS_BLOCK_SIZE / SECTOR_SIZE)

typedef struct {
    char     magic[6];              // "RNAFS1"
    uint16_t reserved;
    uint32_t block_size;            // 4096
    uint32_t total_blocks;

    uint32_t bitmap_start_block;
    uint32_t bitmap_block_count;

    uint32_t dir_start_block;
    uint32_t dir_block_count;

    uint32_t data_start_block;

    uint64_t partition_start_lba;
} Superblock;

typedef struct {
    char     name[32];
    uint8_t  type;          // 0 free, 1 file, 2 dir
    uint8_t  reserved[3];
    uint32_t first_block;
    uint32_t block_count;
    uint32_t size_bytes;
    uint32_t parent_index;
    uint8_t  reserved2[12];
} DirEntry;

void mkfs_rnafs(uint64_t partition_start_lba, uint32_t partition_sector_count);
int rnafs_mount(uint64_t partition_start_lba);
int rnafs_create_file(uint32_t parent, const char *name, uint32_t size_bytes);
int rnafs_read_file(const char *name, void *buffer, uint32_t max_bytes);
int rnafs_write_file(const char *name, const void *buffer, uint32_t size_bytes);
int rnafs_append_file(const char *name, const void *buffer, uint32_t size_bytes);
int rnafs_delete_file(const char *name);
void rnafs_ls();

#endif
