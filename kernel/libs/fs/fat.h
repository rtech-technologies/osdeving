#ifndef FAT_H
#define FAT_H

#include "types.h"

#pragma pack(push, 1)

typedef struct {
    uint8  jmp[3];
    char   oem_name[8];
    uint16 bytes_per_sector;
    uint8  sectors_per_cluster;
    uint16 reserved_sectors;
    uint8  num_fats;
    uint16 root_entry_count;
    uint16 total_sectors_16;
    uint8  media_type;
    uint16 sectors_per_fat;
    uint16 sectors_per_track;
    uint16 num_heads;
    uint32 hidden_sectors;
    uint32 total_sectors_32;
    uint8  drive_number;
    uint8  reserved;
    uint8  boot_signature;
    uint32 volume_id;
    char   volume_label[11];
    char   fs_type[8];
} fat_boot_sector_t;

typedef struct {
    char   name[8];
    char   ext[3];
    uint8  attr;
    uint8  reserved;
    uint8  create_time_tenth;
    uint16 create_time;
    uint16 create_date;
    uint16 last_access_date;
    uint16 first_cluster_high;
    uint16 write_time;
    uint16 write_date;
    uint16 first_cluster_low;
    uint32 file_size;
} fat_dir_entry_t;

#pragma pack(pop)

void mkfs_fat16(uint64 partition_start_lba, uint32 partition_sector_count);

#endif
