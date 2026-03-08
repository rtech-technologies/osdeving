#include "diskman.h"
#include "disk.h"
#include "console.h"
#include "rnafs.h"
#include "kutils.h"
#include "../../include/rsl.h"

static gpt_header_t current_gpt;
static gpt_entry_t  entries[128];

void diskman_init() {
    if (read_sectors(1, 1, &current_gpt)) {
        if (current_gpt.signature == GPT_SIGNATURE) {
            read_sectors(current_gpt.partition_entry_lba,
                         (current_gpt.num_partition_entries * current_gpt.size_partition_entry + 511) / 512,
                         entries);
        }
    }
}

void diskman_add_partition(uint64 start, uint32 count) {
    if (current_gpt.signature != GPT_SIGNATURE) {
        /* Initialize Blank GPT on first use */
        memset(&current_gpt, 0, sizeof(gpt_header_t));
        current_gpt.signature = GPT_SIGNATURE;
        current_gpt.revision = 0x00010000;
        current_gpt.header_size = sizeof(gpt_header_t);
        current_gpt.current_lba = 1;
        current_gpt.first_usable_lba = 34;
        current_gpt.partition_entry_lba = 2;
        current_gpt.num_partition_entries = 128;
        current_gpt.size_partition_entry = 128;
        memset(entries, 0, sizeof(entries));
        print("Diskman: Initializing new GPT...\n");
    }
    for (uint32 i = 0; i < current_gpt.num_partition_entries; i++) {
        int unused = 1;
        for (int j = 0; j < 16; j++) {
            if (entries[i].partition_type_guid.data[j] != 0) {
                unused = 0;
                break;
            }
        }
        if (unused) {
            entries[i].starting_lba = start;
            entries[i].ending_lba = start + count - 1;
            entries[i].partition_type_guid.data[0] = 0x52; /* RNAFS GUID (Fake) */
            entries[i].partition_type_guid.data[1] = 0x4E;
            entries[i].partition_type_guid.data[2] = 0x41;

            /* Update Entry Array CRC */
            current_gpt.partition_entry_array_crc = crc32(entries, (uint64)current_gpt.num_partition_entries * current_gpt.size_partition_entry);

            /* Update Header CRC */
            current_gpt.header_crc = 0;
            current_gpt.header_crc = crc32(&current_gpt, current_gpt.header_size);

            /* Commit to disk */
            write_sectors(current_gpt.partition_entry_lba,
                          (current_gpt.num_partition_entries * current_gpt.size_partition_entry + 511) / 512,
                          entries);
            write_sectors(1, 1, &current_gpt);

            print("Diskman: Added GPT partition ");
            char buf[8];
            itoa(i, buf, 10);
            print(buf);
            print("\n");
            return;
        }
    }
}

void diskman_format_rnafs(int idx) {
    if (idx < 0 || idx >= 128) return;
    if (entries[idx].starting_lba == 0) return;
    uint64 size = entries[idx].ending_lba - entries[idx].starting_lba + 1;
    rnafs_format_partition((uint32)entries[idx].starting_lba, (uint32)size);
}

void diskman_mount_rnafs(int idx) {
    if (idx < 0 || idx >= 128) return;
    if (entries[idx].starting_lba == 0) return;
    rnafs_mount_partition((uint32)entries[idx].starting_lba);
}
