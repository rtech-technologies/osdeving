#include "rnafs.h"
#include "../unice64/kernel.h"
#include "../../include/rsl.h"

static rnafs_superblock_t* sb = 0;
static uint8* bitmap = 0;

void rnafs_init() {
    if (!kboot_params.ramdisk_base) return;

    sb = (rnafs_superblock_t*)kboot_params.ramdisk_base;

    if (sb->magic != RNAFS_MAGIC) {
        /* If not formatted, we don't mount.
           In a real system, we'd have a 'format' service. */
        sb = 0;
        return;
    }

    bitmap = (uint8*)kboot_params.ramdisk_base + sb->bitmap_start * RNAFS_BLOCK_SIZE;
}

static int is_block_free(uint64 block) {
    if (!bitmap) return 0;
    return !(bitmap[block / 8] & (1 << (block % 8)));
}

static void set_block_used(uint64 block) {
    if (!bitmap) return;
    bitmap[block / 8] |= (1 << (block % 8));
}

static uint64 find_free_blocks(uint64 count) {
    if (!sb) return 0;

    uint64 consecutive = 0;
    uint64 start = 0;

    for (uint64 i = sb->data_start; i < sb->total_blocks; i++) {
        if (consecutive == 0) start = i;

        if (is_block_free(i)) {
            consecutive++;
            if (consecutive == count) return start;
        } else {
            consecutive = 0;
        }
    }
    return 0;
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

    /* Compat for v0 shell.bin */
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

    /* 1. Find existing file */
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] != 0 && strcmp(dir[i].name, path) == 0) {
            uint64 blocks_needed = (size + RNAFS_BLOCK_SIZE - 1) / RNAFS_BLOCK_SIZE;
            uint64 current_blocks = (dir[i].size + RNAFS_BLOCK_SIZE - 1) / RNAFS_BLOCK_SIZE;

            if (blocks_needed <= current_blocks) {
                memcpy((uint8*)kboot_params.ramdisk_base + dir[i].start_block * RNAFS_BLOCK_SIZE, buffer, size);
                dir[i].size = size;
                return (INTN)size;
            }
            return -1; /* For v1 we don't reallocate existing files yet */
        }
    }

    /* 2. Create new file if space exists */
    int free_idx = -1;
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] == 0) {
            free_idx = i;
            break;
        }
    }

    if (free_idx != -1) {
        uint64 blocks_needed = (size + RNAFS_BLOCK_SIZE - 1) / RNAFS_BLOCK_SIZE;
        uint64 start = find_free_blocks(blocks_needed);

        if (start != 0) {
            /* Mark bitmap */
            for (uint64 b = 0; b < blocks_needed; b++) {
                set_block_used(start + b);
            }

            /* Fill entry */
            strcpy(dir[free_idx].name, path);
            dir[free_idx].start_block = start;
            dir[free_idx].size = size;

            /* Write data */
            memcpy((uint8*)kboot_params.ramdisk_base + start * RNAFS_BLOCK_SIZE, buffer, size);
            return (INTN)size;
        }
    }

    return -1;
}
