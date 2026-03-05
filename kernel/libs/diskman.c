#include "diskman.h"
#include "disk.h"
#include "console.h"
#include "rnafs.h"
#include "../../include/rsl.h"

static mbr_t current_mbr;

void diskman_init() {
    if (read_sectors(0, 1, &current_mbr)) {
        if (current_mbr.signature != 0xAA55) {
            memset(&current_mbr, 0, sizeof(mbr_t));
            current_mbr.signature = 0xAA55;
            write_sectors(0, 1, &current_mbr);
        }
    }
}

void diskman_add_partition(uint64 start, uint32 count) {
    for (int i = 0; i < 4; i++) {
        if (current_mbr.partitions[i].size_in_sectors == 0) {
            current_mbr.partitions[i].starting_lba = (uint32)start;
            current_mbr.partitions[i].size_in_sectors = count;
            current_mbr.partitions[i].os_type = 0x82;
            write_sectors(0, 1, &current_mbr);
            print("Diskman: Added partition ");
            char buf[8];
            itoa(i, buf, 10);
            print(buf);
            print("\n");
            return;
        }
    }
}

void diskman_format_rnafs(int idx) {
    if (idx < 0 || idx >= 4) return;
    if (current_mbr.partitions[idx].size_in_sectors == 0) return;
    rnafs_format_partition(current_mbr.partitions[idx].starting_lba, current_mbr.partitions[idx].size_in_sectors);
}

void diskman_mount_rnafs(int idx) {
    if (idx < 0 || idx >= 4) return;
    if (current_mbr.partitions[idx].size_in_sectors == 0) return;
    rnafs_mount_partition(current_mbr.partitions[idx].starting_lba);
}
