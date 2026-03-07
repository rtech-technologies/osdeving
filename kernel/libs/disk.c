#include "disk.h"
#include "kutils.h"
#include "../unice64/kernel.h"
#include "../../include/rsl.h"

void disk_init() {
}

int read_sectors(uint64 lba, uint32 count, void* buffer) {
    if (!kboot_params.ramdisk_base) return 0;
    uint64 offset = lba * 512;
    uint64 size = (uint64)count * 512;
    if (offset + size > kboot_params.ramdisk_size) return 0;
    memcpy(buffer, (uint8*)kboot_params.ramdisk_base + offset, size);
    return 1;
}

int write_sectors(uint64 lba, uint32 count, const void* buffer) {
    if (!kboot_params.ramdisk_base) return 0;
    uint64 offset = lba * 512;
    uint64 size = (uint64)count * 512;
    if (offset + size > kboot_params.ramdisk_size) return 0;
    memcpy((uint8*)kboot_params.ramdisk_base + offset, buffer, size);
    return 1;
}
