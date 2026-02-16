#include "disk.h"
#include "../kernel/kernel.h"

void disk_init() {
    /* Ramdisk is already in memory */
}

int read_sectors(uint64 lba, uint32 count, void* buffer) {
    if (!kboot_params.ramdisk_base) return 0;

    uint64 offset = lba * 512;
    uint64 size = (uint64)count * 512;

    if (offset + size > kboot_params.ramdisk_size) return 0;

    uint8* src = (uint8*)kboot_params.ramdisk_base + offset;
    uint8* dst = (uint8*)buffer;

    for (uint64 i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    return 1;
}

int write_sectors(uint64 lba, uint32 count, const void* buffer) {
    if (!kboot_params.ramdisk_base) return 0;

    uint64 offset = lba * 512;
    uint64 size = (uint64)count * 512;

    if (offset + size > kboot_params.ramdisk_size) return 0;

    const uint8* src = (const uint8*)buffer;
    uint8* dst = (uint8*)kboot_params.ramdisk_base + offset;

    for (uint64 i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    return 1;
}
