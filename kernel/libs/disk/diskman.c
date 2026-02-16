#include "diskman.h"
#include "console.h"
#include "../fs/rnafs.h"
#include "../fs/fat.h"

typedef struct {
    uint64 start_lba;
    uint32 sector_count;
    int type; // 1 for RNAFS
} partition_info_t;

#define MAX_PARTITIONS 4
static partition_info_t partitions[MAX_PARTITIONS];
static int partition_count = 0;

void diskman_init() {
    // Mock partition discovery
    // Assume a 4MB partition starts at 1MB (2048 sectors)
    partitions[0].start_lba = 2048;
    partitions[0].sector_count = 8192; // 4 MB
    partitions[0].type = 1;
    partition_count = 1;
}

void diskman_format() {
    if (partition_count > 0) {
        mkfs_rnafs(partitions[0].start_lba, partitions[0].sector_count);
    }
}

void diskman_mount() {
    if (partition_count > 0) {
        rnafs_mount(partitions[0].start_lba);
    }
}

void diskman_ls() {
    console_print("--- Disk Inventory ---\n");
    for (int i = 0; i < partition_count; i++) {
        console_print("Partition ");
        // Simplified index print
        char buf[2]; buf[0] = '0' + i; buf[1] = 0;
        console_print(buf);
        console_print(": ");
        if (partitions[i].type == 1) console_print("RNAFS");
        else if (partitions[i].type == 2) console_print("FAT16");
        else console_print("UNKNOWN");
        console_print("\n");
    }
    rnafs_ls();
}

void diskman_add_partition(uint64 start_lba, uint32 sector_count) {
    if (partition_count < MAX_PARTITIONS) {
        partitions[partition_count].start_lba = start_lba;
        partitions[partition_count].sector_count = sector_count;
        partitions[partition_count].type = 0; // unknown
        partition_count++;
        console_print("New partition added.\n");
    }
}

void diskman_format_fat(int idx) {
    if (idx >= 0 && idx < partition_count) {
        mkfs_fat16(partitions[idx].start_lba, partitions[idx].sector_count);
        partitions[idx].type = 2; // FAT16
    }
}
