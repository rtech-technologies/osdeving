#include "diskman.h"
#include "disk.h"
#include "console.h"
#include "../fs/fat.h"
#include "../fs/rnafs.h"

typedef struct __attribute__((packed)) {
    uint8  boot_indicator;
    uint8  starting_chs[3];
    uint8  system_id;
    uint8  ending_chs[3];
    uint32 relative_sector;
    uint32 total_sectors;
} MBR_Partition;

typedef struct __attribute__((packed)) {
    uint8  bootstrap[446];
    MBR_Partition partitions[4];
    uint16 signature;
} MBR;

void diskman_init() {
    print("Diskman: Initializing...\n");
    disk_init();
    diskman_partition_efs();
}

void diskman_partition_efs() {
    uint8 buffer[512];
    if (!read_sectors(0, 1, buffer)) return;

    MBR* mbr = (MBR*)buffer;
    if (mbr->signature != 0xAA55) {
        print("Diskman: Initializing MBR and EFS...\n");
        for (int i = 0; i < 512; i++) buffer[i] = 0;
        mbr = (MBR*)buffer;
        mbr->partitions[0].system_id = 0xEF; /* ESP */
        mbr->partitions[0].relative_sector = 2048; /* Start at 1MB */
        mbr->partitions[0].total_sectors = 32768;  /* 16MB */
        mbr->signature = 0xAA55;
        write_sectors(0, 1, buffer);
    }

    /* Auto-mount FAT on Partition 0 if it exists */
    if (mbr->partitions[0].system_id == 0xEF) {
        fat_mount(mbr->partitions[0].relative_sector);
    }
}

void diskman_add_partition(uint64 start, uint32 count) {
    uint8 buffer[512];
    if (!read_sectors(0, 1, buffer)) return;

    MBR* mbr = (MBR*)buffer;
    if (mbr->signature != 0xAA55) return;

    /* Find free slot (skipping 0 which is for EFI) */
    for (int i = 1; i < 4; i++) {
        if (mbr->partitions[i].system_id == 0) {
            mbr->partitions[i].system_id = 0x7F; /* Custom RNAFS ID */
            mbr->partitions[i].relative_sector = (uint32)start;
            mbr->partitions[i].total_sectors = count;
            write_sectors(0, 1, buffer);
            print("Diskman: Added partition at slot ");
            char s[2] = {'0' + i, 0};
            print(s);
            print("\n");
            return;
        }
    }
    print("Diskman: No free partition slots.\n");
}

void diskman_format_rnafs(int idx) {
    if (idx < 0 || idx > 3) return;
    uint8 buffer[512];
    if (!read_sectors(0, 1, buffer)) return;
    MBR* mbr = (MBR*)buffer;

    if (mbr->partitions[idx].system_id != 0x7F) {
        print("Diskman: Partition is not RNAFS type.\n");
        return;
    }

    mkfs_rnafs(mbr->partitions[idx].relative_sector, mbr->partitions[idx].total_sectors);
}

void diskman_mount_rnafs(int idx) {
    if (idx < 0 || idx > 3) return;
    uint8 buffer[512];
    if (!read_sectors(0, 1, buffer)) return;
    MBR* mbr = (MBR*)buffer;

    if (mbr->partitions[idx].system_id != 0x7F) {
        print("Diskman: Partition is not RNAFS type.\n");
        return;
    }

    if (rnafs_mount(mbr->partitions[idx].relative_sector)) {
        print("Diskman: RNAFS mounted from partition ");
        char s[2] = {'0' + idx, 0};
        print(s);
        print("\n");
    } else {
        print("Diskman: RNAFS mount failed.\n");
    }
}
