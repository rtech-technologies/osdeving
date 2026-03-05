#ifndef DISKMAN_H
#define DISKMAN_H

#include "../../include/types.h"

/* GPT (GUID Partition Table) Structures - 64-bit */

typedef struct {
    uint8 data[16];
} guid_t;

typedef struct {
    uint64 signature;     /* "EFI PART" */
    uint32 revision;
    uint32 header_size;
    uint32 header_crc;
    uint32 reserved;
    uint64 current_lba;
    uint64 backup_lba;
    uint64 first_usable_lba;
    uint64 last_usable_lba;
    guid_t disk_guid;
    uint64 partition_entry_lba;
    uint32 num_partition_entries;
    uint32 size_partition_entry;
    uint32 partition_entry_array_crc;
} __attribute__((packed)) gpt_header_t;

typedef struct {
    guid_t partition_type_guid;
    guid_t unique_partition_guid;
    uint64 starting_lba;
    uint64 ending_lba;
    uint64 attributes;
    uint16 partition_name[36];
} __attribute__((packed)) gpt_entry_t;

#define GPT_SIGNATURE 0x5452415020494645ULL /* "EFI PART" */

void diskman_init();
void diskman_add_partition(uint64 start, uint32 count);
void diskman_format_rnafs(int idx);
void diskman_mount_rnafs(int idx);

#endif
