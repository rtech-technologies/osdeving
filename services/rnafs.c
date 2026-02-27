#include "rnafs.h"
#include "../kernel/kernel.h"
#include "../include/utils.h"

static rnafs_superblock_t* sb = 0;

void rnafs_init() {
    if (!kboot_params.ramdisk_base) return;

    /* RNAFS Superblock is at the start of the ramdisk */
    sb = (rnafs_superblock_t*)kboot_params.ramdisk_base;

    if (sb->magic != RNAFS_MAGIC) {
        /* Not an RNAFS disk - could format here, but for now we expect it */
        sb = 0;
    }
}

INTN rnafs_read(const char* path, void* buffer, uint64 max_size) {
    if (!sb || !kboot_params.ramdisk_base) return -1;

    rnafs_entry_t* dir = (rnafs_entry_t*)((uint8*)kboot_params.ramdisk_base + sb->dir_start * RNAFS_BLOCK_SIZE);

    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] == 0) continue;
        if (strcmp(dir[i].name, path) == 0) {
            uint64 size = dir[i].size;
            if (size > max_size) size = max_size;
            memcpy(buffer, (uint8*)kboot_params.ramdisk_base + dir[i].start_block * RNAFS_BLOCK_SIZE, size);
            return (INTN)size;
        }
    }

    /* Fallback for shell.bin if not in directory (for v0 compatibility) */
    if (strcmp(path, "shell.bin") == 0) {
        uint64 size = kboot_params.ramdisk_size;
        if (size > max_size) size = max_size;
        memcpy(buffer, kboot_params.ramdisk_base, size);
        return (INTN)size;
    }

    return -1;
}

INTN rnafs_write(const char* path, const void* buffer, uint64 size) {
    if (!sb || !kboot_params.ramdisk_base) return -1;

    rnafs_entry_t* dir = (rnafs_entry_t*)((uint8*)kboot_params.ramdisk_base + sb->dir_start * RNAFS_BLOCK_SIZE);

    /* Find existing file */
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] != 0 && strcmp(dir[i].name, path) == 0) {
            /* For v1 contiguous, we only overwrite if it fits in same blocks */
            uint64 blocks_needed = (size + RNAFS_BLOCK_SIZE - 1) / RNAFS_BLOCK_SIZE;
            uint64 current_blocks = (dir[i].size + RNAFS_BLOCK_SIZE - 1) / RNAFS_BLOCK_SIZE;

            if (blocks_needed <= current_blocks) {
                memcpy((uint8*)kboot_params.ramdisk_base + dir[i].start_block * RNAFS_BLOCK_SIZE, buffer, size);
                dir[i].size = size;
                return (INTN)size;
            }
            return -1; /* Allocation not supported in this simple driver */
        }
    }

    return -1;
}
