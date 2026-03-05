#ifndef DISKMAN_H
#define DISKMAN_H

#include "../../include/types.h"

typedef struct {
    uint8  boot_indicator;
    uint8  starting_chs[3];
    uint8  os_type;
    uint8  ending_chs[3];
    uint32 starting_lba;
    uint32 size_in_sectors;
} __attribute__((packed)) mbr_partition_t;

typedef struct {
    uint8  bootstrap[446];
    mbr_partition_t partitions[4];
    uint16 signature;
} __attribute__((packed)) mbr_t;

void diskman_init();
void diskman_add_partition(uint64 start, uint32 count);
void diskman_format_rnafs(int idx);
void diskman_mount_rnafs(int idx);

#endif
