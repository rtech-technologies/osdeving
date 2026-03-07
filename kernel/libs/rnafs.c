#include "rnafs.h"
#include "disk.h"
#include "console.h"
#include "kutils.h"
#include "../unice64/kernel.h"
#include "../../include/rsl.h"

static uint32 mounted_lba = 0;
static rnafs_superblock_t active_sb;
static int is_mounted = 0;

void rnafs_init() {
    is_mounted = 0;
}

void rnafs_format_partition(uint32 start_lba, uint32 size_sectors) {
    rnafs_superblock_t sb;
    sb.magic = RNAFS_MAGIC;
    sb.total_blocks = size_sectors;
    sb.bitmap_start = 1;
    sb.bitmap_blocks = 1;
    sb.dir_start = 2;
    sb.dir_blocks = 4;
    sb.data_start = 6;
    write_sectors(start_lba, 1, &sb);
    uint8 zero[512];
    memset(zero, 0, 512);
    for (int i = 1; i < 6; i++) {
        write_sectors(start_lba + i, 1, zero);
    }
}

void rnafs_mount_partition(uint32 start_lba) {
    if (read_sectors(start_lba, 1, &active_sb)) {
        if (active_sb.magic == RNAFS_MAGIC) {
            mounted_lba = start_lba;
            is_mounted = 1;
            print("RNAFS: Mounted.\n");
        }
    }
}

void rnafs_ls() {
    if (!is_mounted) return;
    rnafs_entry_t entries[RNAFS_MAX_FILES];
    if (!read_sectors(mounted_lba + active_sb.dir_start, (uint32)active_sb.dir_blocks, entries)) return;
    print("Files:\n");
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (entries[i].name[0] != 0) {
            print("  ");
            print(entries[i].name);
            print("\n");
        }
    }
}

INTN rnafs_read(const char* path, void* buffer, uint64 max_size) {
    if (!is_mounted) return -1;
    rnafs_entry_t entries[RNAFS_MAX_FILES];
    if (!read_sectors(mounted_lba + active_sb.dir_start, (uint32)active_sb.dir_blocks, entries)) return -1;
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (entries[i].name[0] != 0 && strcmp(entries[i].name, path) == 0) {
            uint64 size = entries[i].size;
            if (size > max_size) size = max_size;
            uint32 blocks = (uint32)((size + 511) / 512);
            if (read_sectors(mounted_lba + entries[i].start_block, blocks, buffer)) {
                return (INTN)size;
            }
        }
    }
    return -1;
}

INTN rnafs_write(const char* path, const void* buffer, uint64 size) {
    if (!is_mounted) return -1;
    rnafs_entry_t entries[RNAFS_MAX_FILES];
    if (!read_sectors(mounted_lba + active_sb.dir_start, (uint32)active_sb.dir_blocks, entries)) return -1;
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (entries[i].name[0] != 0 && strcmp(entries[i].name, path) == 0) {
             uint32 blocks = (uint32)((size + 511) / 512);
             if (write_sectors(mounted_lba + entries[i].start_block, blocks, buffer)) {
                 entries[i].size = size;
                 write_sectors(mounted_lba + active_sb.dir_start, (uint32)active_sb.dir_blocks, entries);
                 return (INTN)size;
             }
             return -1;
        }
    }
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (entries[i].name[0] == 0) {
            uint64 start = active_sb.data_start + (i * 128);
            strcpy(entries[i].name, path);
            entries[i].start_block = start;
            entries[i].size = size;
            uint32 blocks = (uint32)((size + 511) / 512);
            if (write_sectors(mounted_lba + start, blocks, buffer)) {
                write_sectors(mounted_lba + active_sb.dir_start, (uint32)active_sb.dir_blocks, entries);
                return (INTN)size;
            }
            return -1;
        }
    }
    return -1;
}
