#include "diskman.h"
#include "console.h"
#include "../fs/rnafs.h"
#include "../fs/fat.h"

typedef struct {
    uint64 start_lba;
    uint32 sector_count;
    int type; // 1: RNAFS, 2: FAT16, 3: PROTECTED_EFS
} partition_info_t;

#define MAX_PARTITIONS 4
static partition_info_t partitions[MAX_PARTITIONS];
static int partition_count = 0;

void diskman_init() {
    // Partition 0 is always the EFI System Partition (EFS)
    // We assume it occupies the first 1MB (2048 sectors)
    partitions[0].start_lba = 0;
    partitions[0].sector_count = 2048;
    partitions[0].type = 3; // PROTECTED_EFS
    partition_count = 1;

    // Example available partition starting after EFS
    partitions[1].start_lba = 2048;
    partitions[1].sector_count = 8192; // 4 MB
    partitions[1].type = 0; // UNKNOWN
    partition_count = 2;
}

void diskman_format() {
    // Current 'format' command in shell calls this for RNAFS
    // For v0, we assume it's for partition 1 (the first non-EFS one)
    if (partition_count > 1) {
        if (partitions[1].type == 3) {
            console_print("Error: Target partition is PROTECTED.\n");
            return;
        }
        mkfs_rnafs(partitions[1].start_lba, partitions[1].sector_count);
        partitions[1].type = 1;
    }
}

void diskman_mount(int idx) {
    if (idx >= 0 && idx < partition_count) {
        if (partitions[idx].type == 1) { // RNAFS
            rnafs_mount(partitions[idx].start_lba);
        } else {
            console_print("Error: Partition is not RNAFS.\n");
        }
    }
}

void diskman_ls() {
    console_print("--- Disk Inventory ---\n");
    for (int i = 0; i < partition_count; i++) {
        console_print("Partition ");
        char buf[2]; buf[0] = '0' + i; buf[1] = 0;
        console_print(buf);
        console_print(": ");
        if (partitions[i].type == 1) console_print("RNAFS");
        else if (partitions[i].type == 2) console_print("FAT16");
        else if (partitions[i].type == 3) console_print("EFS (PROTECTED)");
        else console_print("UNKNOWN");
        console_print("\n");
    }
    rnafs_ls();
}

void diskman_add_partition(uint64 start_lba, uint32 sector_count) {
    // Check for overlap with EFS (Partition 0)
    if (start_lba < (partitions[0].start_lba + partitions[0].sector_count)) {
        console_print("Error: Partition overlaps with PROTECTED EFS.\n");
        return;
    }

    if (partition_count < MAX_PARTITIONS) {
        partitions[partition_count].start_lba = start_lba;
        partitions[partition_count].sector_count = sector_count;
        partitions[partition_count].type = 0;
        partition_count++;
        console_print("New partition added.\n");
    } else {
        console_print("Error: Maximum partitions reached.\n");
    }
}

void diskman_format_fat(int idx) {
    if (idx >= 0 && idx < partition_count) {
        if (partitions[idx].type == 3) {
            console_print("Error: Cannot format PROTECTED partition.\n");
            return;
        }
        mkfs_fat16(partitions[idx].start_lba, partitions[idx].sector_count);
        partitions[idx].type = 2;
    } else {
        console_print("Error: Invalid partition index.\n");
    }
}
