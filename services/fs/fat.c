#include "fat.h"
#include "../io/disk.h"
#include "../io/console.h"

typedef struct __attribute__((packed)) {
    uint8  jmp[3];
    uint8  oem[8];
    uint16 bytes_per_sector;
    uint8  sectors_per_cluster;
    uint16 reserved_sectors;
    uint8  fats;
    uint16 root_entries;
    uint16 total_sectors_short;
    uint8  media_type;
    uint16 fat_size_16;
    uint16 sectors_per_track;
    uint16 heads;
    uint32 hidden_sectors;
    uint32 total_sectors_long;
} FAT_BPB;

void fat_init() {
    /* Registry Ritual */
}

int fat_mount(uint64 partition_start_lba) {
    uint8 buffer[512];
    if (!read_sectors(partition_start_lba, 1, buffer)) return 0;

    FAT_BPB* bpb = (FAT_BPB*)buffer;

    /* Very basic check: bytes per sector should be 512 */
    if (bpb->bytes_per_sector != 512) return 0;

    /* Check for some FAT signatures if needed, but for v0 this is enough to "init" */
    print("FATFS: Partition identified at LBA ");
    /* Simple int to string would be good here but print is basic */
    print("...\n");

    return 1;
}
