#include "rnafs.h"
#include "../kernel/kernel.h"
#include "../include/utils.h"

/* RNAFS v0: Simple single-file in-memory filesystem */
/* Directory table at end of ramdisk, after actual file data */
#define RNAFS_DIR_OFFSET (1024 * 1024)  /* 1MB into ramdisk for directory */

static rnafs_entry_t* get_directory() {
    if (!kboot_params.ramdisk_base || kboot_params.ramdisk_size < RNAFS_DIR_OFFSET + sizeof(rnafs_entry_t)) {
        return NULL;
    }
    return (rnafs_entry_t*)((uint8*)kboot_params.ramdisk_base + RNAFS_DIR_OFFSET);
}

void rnafs_init() {
    /* Initialize directory entries to empty */
    rnafs_entry_t* dir = get_directory();
    if (dir) {
        for (int i = 0; i < RNAFS_MAX_FILES; i++) {
            dir[i].name[0] = 0;
            dir[i].offset = 0;
            dir[i].size = 0;
            dir[i].flags = 0;
        }
        /* Register shell.bin as first file */
        if (kboot_params.ramdisk_base && kboot_params.ramdisk_size > 0) {
            char_strncpy(dir[0].name, "shell.bin", 64);
            dir[0].offset = 0;
            dir[0].size = kboot_params.ramdisk_size > RNAFS_DIR_OFFSET ? RNAFS_DIR_OFFSET : kboot_params.ramdisk_size;
            dir[0].flags = 0;
        }
    }
}

INTN rnafs_read(const char* path, void* buffer, uint64 max_size) {
    if (!kboot_params.ramdisk_base || !buffer) return -1;

    rnafs_entry_t* dir = get_directory();
    if (!dir) return -1;

    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] == 0) continue;
        if (strcmp(dir[i].name, path) == 0) {
            uint64 size = dir[i].size;
            if (size > max_size) size = max_size;
            if (dir[i].offset + size > kboot_params.ramdisk_size) {
                size = kboot_params.ramdisk_size - dir[i].offset;
            }
            memcpy(buffer, (uint8*)kboot_params.ramdisk_base + dir[i].offset, size);
            return (INTN)size;
        }
    }

    return -1;
}

INTN rnafs_write(const char* path, const void* buffer, uint64 size) {
    if (!kboot_params.ramdisk_base || !buffer) return -1;

    rnafs_entry_t* dir = get_directory();
    if (!dir) return -1;

    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] != 0 && strcmp(dir[i].name, path) == 0) {
            if (dir[i].offset + size > RNAFS_DIR_OFFSET) {
                return -1; /* Not enough space */
            }
            memcpy((uint8*)kboot_params.ramdisk_base + dir[i].offset, buffer, size);
            dir[i].size = size;
            return (INTN)size;
        }
    }

    return -1;
}
