#include "diskman.h"
#include "disk.h"
#include "console.h"
#include "../fs/fat.h"

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
    print("Diskman: Initializing all drives...\n");
    disk_init();

    /* Auto-partition EFS if requested */
    diskman_partition_efs();
}

void diskman_partition_efs() {
    uint8 buffer[512];
    if (!read_sectors(0, 1, buffer)) return;

    MBR* mbr = (MBR*)buffer;
    if (mbr->signature != 0xAA55) {
        print("Diskman: Partition table not found. Creating EFS partition...\n");

        /* Clear MBR */
        for (int i = 0; i < 512; i++) buffer[i] = 0;
        mbr = (MBR*)buffer;

        /* Create 1st partition: EFI System Partition (0xEF) */
        mbr->partitions[0].boot_indicator = 0x00;
        mbr->partitions[0].system_id = 0xEF;
        mbr->partitions[0].relative_sector = 1;
        mbr->partitions[0].total_sectors = 2048; /* Minimal EFS size for v0 */

        mbr->signature = 0xAA55;

        write_sectors(0, 1, buffer);
        print("Diskman: EFS Partitioned at LBA 1\n");
    } else {
        print("Diskman: Partition table found.\n");
    }

    /* Initialize FATFS on the identified partition(s) */
    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].system_id == 0xEF || mbr->partitions[i].system_id == 0x01 || mbr->partitions[i].system_id == 0x06 || mbr->partitions[i].system_id == 0x0E) {
            fat_mount(mbr->partitions[i].relative_sector);
        }
    }
}
