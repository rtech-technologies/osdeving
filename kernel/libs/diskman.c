#include "diskman.h"
#include "disk.h"
#include "console.h"
#include "rnafs.h"
#include "../../include/rsl.h"

static gpt_header_t current_gpt;
static gpt_entry_t  entries[128]; /* GPT standard usually has 128 entries */

void diskman_init() {
    /* GPT Header is at LBA 1 */
    if (read_sectors(1, 1, &current_gpt)) {
        if (current_gpt.signature == GPT_SIGNATURE) {
            /* Read partition entries */
            read_sectors(current_gpt.partition_entry_lba,
                         (current_gpt.num_partition_entries * current_gpt.size_partition_entry + 511) / 512,
                         entries);
        } else {
            print("Diskman: No valid GPT found. Initialization required.\n");
        }
    }
}

void diskman_add_partition(uint64 start, uint32 count) {
    if (current_gpt.signature != GPT_SIGNATURE) {
        print("Diskman: Error - No GPT disk initialized.\n");
        return;
    }

    for (uint32 i = 0; i < current_gpt.num_partition_entries; i++) {
        /* Check for unused entry (Type GUID = 0) */
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
            /* OSx2 Proprietary RNAFS Type GUID (Randomly generated for this OS) */
            entries[i].partition_type_guid.data[0] = 0x52; /* 'R' */
            entries[i].partition_type_guid.data[1] = 0x4E; /* 'N' */
            entries[i].partition_type_guid.data[2] = 0x41; /* 'A' */

            /* Update disk */
            write_sectors(current_gpt.partition_entry_lba,
                          (current_gpt.num_partition_entries * current_gpt.size_partition_entry + 511) / 512,
                          entries);

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
