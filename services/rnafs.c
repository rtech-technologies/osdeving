#include "rnafs.h"
#include "../kernel/kernel.h"
#include "../include/utils.h"

void rnafs_init() {
    /* RNAFS uses the ramdisk passed in boot_params */
}

INTN rnafs_read(const char* path, void* buffer, uint64 max_size) {
    if (!kboot_params.ramdisk_base) return -1;

    /* For v0, if we only have shell.bin in ramdisk, we treat offset 0 as shell.bin */
    if (strcmp(path, "shell.bin") == 0) {
        uint64 size = kboot_params.ramdisk_size;
        if (size > max_size) size = max_size;
        memcpy(buffer, kboot_params.ramdisk_base, size);
        return (INTN)size;
    }

    /* Search for other files in a simple table at the start of ramdisk */
    rnafs_entry_t* dir = (rnafs_entry_t*)kboot_params.ramdisk_base;
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] == 0) continue;
        if (strcmp(dir[i].name, path) == 0) {
            uint64 size = dir[i].size;
            if (size > max_size) size = max_size;
            memcpy(buffer, (uint8*)kboot_params.ramdisk_base + dir[i].offset, size);
            return (INTN)size;
        }
    }

    return -1;
}

INTN rnafs_write(const char* path, const void* buffer, uint64 size) {
    if (!kboot_params.ramdisk_base) return -1;

    rnafs_entry_t* dir = (rnafs_entry_t*)kboot_params.ramdisk_base;
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (dir[i].name[0] != 0 && strcmp(dir[i].name, path) == 0) {
            memcpy((uint8*)kboot_params.ramdisk_base + dir[i].offset, buffer, size);
            dir[i].size = size;
            return (INTN)size;
        }
    }

    return -1;
}
