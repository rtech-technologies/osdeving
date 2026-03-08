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

    uint8 block[512];
    memset(block, 0, 512);

    /* Bitmap: mark metadata blocks (0-5) as used */
    block[0] = 0x3F; /* 00111111 */
    write_sectors(start_lba + (uint32)sb.bitmap_start, 1, block);

    /* Directory & Data Initialization */
    memset(block, 0, 512);
    for (int i = (int)sb.dir_start; i < (int)sb.data_start; i++) {
        write_sectors(start_lba + (uint32)i, 1, block);
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
    memset(entries, 0, sizeof(entries));
    if (!read_sectors(mounted_lba + (uint32)active_sb.dir_start, (uint32)active_sb.dir_blocks, entries)) return;
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
    memset(entries, 0, sizeof(entries));
    if (!read_sectors(mounted_lba + (uint32)active_sb.dir_start, (uint32)active_sb.dir_blocks, entries)) return -1;
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
    memset(entries, 0, sizeof(entries));
    if (!read_sectors(mounted_lba + (uint32)active_sb.dir_start, (uint32)active_sb.dir_blocks, entries)) return -1;

    uint8 bitmap[512];
    if (!read_sectors(mounted_lba + (uint32)active_sb.bitmap_start, 1, bitmap)) return -1;

    uint32 blocks_needed = (uint32)((size + 511) / 512);
    int entry_idx = -1;
    uint64 start_block = 0;

    /* Check if file exists */
    for (int i = 0; i < RNAFS_MAX_FILES; i++) {
        if (entries[i].name[0] != 0 && strcmp(entries[i].name, path) == 0) {
            /* For v0 simplicity, we re-allocate even for existing files to avoid complexity */
            /* In a real FS we'd check if old space fits. Here we just clear old bits if we were fancy. */
            entry_idx = i;
            break;
        }
    }

    if (entry_idx == -1) {
        /* Find new entry */
        for (int i = 0; i < RNAFS_MAX_FILES; i++) {
            if (entries[i].name[0] == 0) {
                entry_idx = i;
                break;
            }
        }
    }

    if (entry_idx == -1) return -1;

    /* Simple Contiguous Allocation */
    uint32 count = 0;
    for (uint32 i = (uint32)active_sb.data_start; i < (uint32)active_sb.total_blocks; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            if (count == 0) start_block = i;
            count++;
            if (count == blocks_needed) break;
        } else {
            count = 0;
        }
    }

    if (count < blocks_needed) return -1;

    /* Mark Bitmap */
    for (uint32 i = (uint32)start_block; i < (uint32)start_block + blocks_needed; i++) {
        bitmap[i / 8] |= (1 << (i % 8));
    }

    /* Write Data */
    if (!write_sectors(mounted_lba + (uint32)start_block, blocks_needed, buffer)) return -1;

    /* Update Directory & Bitmap */
    strcpy(entries[entry_idx].name, path);
    entries[entry_idx].start_block = start_block;
    entries[entry_idx].size = size;

    write_sectors(mounted_lba + (uint32)active_sb.bitmap_start, 1, bitmap);
    write_sectors(mounted_lba + (uint32)active_sb.dir_start, (uint32)active_sb.dir_blocks, entries);

    return (INTN)size;
}
