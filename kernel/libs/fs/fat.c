#include "fat.h"
#include "../disk/disk.h"
#include "console.h"
#include "memory.h"

static void* memset(void* s, int c, UINTN n) {
    uint8* p = s;
    while (n--) *p++ = (uint8)c;
    return s;
}

static void* memcpy(void* dest, const void* src, UINTN n) {
    uint8* d = dest;
    const uint8* s = src;
    for (UINTN i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void mkfs_fat16(uint64 partition_start_lba, uint32 partition_sector_count) {
    console_print("Formatting FAT16 partition...\n");

    uint8 sector_buf[512];
    memset(sector_buf, 0, 512);

    fat_boot_sector_t* bpb = (fat_boot_sector_t*)sector_buf;

    bpb->jmp[0] = 0xEB;
    bpb->jmp[1] = 0x3C;
    bpb->jmp[2] = 0x90;
    memcpy(bpb->oem_name, "RTECHDOS", 8);
    bpb->bytes_per_sector = 512;
    bpb->sectors_per_cluster = 8; // 4KB clusters
    bpb->reserved_sectors = 1;
    bpb->num_fats = 2;
    bpb->root_entry_count = 512;

    if (partition_sector_count < 65535) {
        bpb->total_sectors_16 = (uint16)partition_sector_count;
        bpb->total_sectors_32 = 0;
    } else {
        bpb->total_sectors_16 = 0;
        bpb->total_sectors_32 = partition_sector_count;
    }

    bpb->media_type = 0xF8;

    // Calculate sectors per FAT
    // total clusters = total_sectors / sectors_per_cluster
    // fat_size_bytes = total_clusters * 2 (for FAT16)
    // sectors_per_fat = (fat_size_bytes + 511) / 512
    uint32 total_clusters = partition_sector_count / 8;
    uint32 fat_size_sectors = (total_clusters * 2 + 511) / 512;
    bpb->sectors_per_fat = (uint16)fat_size_sectors;

    bpb->sectors_per_track = 32;
    bpb->num_heads = 64;
    bpb->hidden_sectors = 0;
    bpb->drive_number = 0x80;
    bpb->boot_signature = 0x29;
    bpb->volume_id = 0x12345678;
    memcpy(bpb->volume_label, "OSX2 VOLUME", 11);
    memcpy(bpb->fs_type, "FAT16   ", 8);

    // Write Boot Sector
    write_sectors(partition_start_lba, 1, sector_buf);

    // Clear FATs
    memset(sector_buf, 0, 512);
    // Initial FAT entries: Media descriptor and EOF for clusters 0 and 1
    sector_buf[0] = 0xF8;
    sector_buf[1] = 0xFF;
    sector_buf[2] = 0xFF;
    sector_buf[3] = 0xFF;

    for (int f = 0; f < 2; f++) {
        uint64 fat_start = partition_start_lba + bpb->reserved_sectors + (f * fat_size_sectors);
        write_sectors(fat_start, 1, sector_buf);

        memset(sector_buf, 0, 512);
        for (uint32 s = 1; s < fat_size_sectors; s++) {
            write_sectors(fat_start + s, 1, sector_buf);
        }
        // Restore initial entries for the loop (though it's only written once per FAT)
        sector_buf[0] = 0xF8;
        sector_buf[1] = 0xFF;
        sector_buf[2] = 0xFF;
        sector_buf[3] = 0xFF;
    }

    // Clear Root Directory
    memset(sector_buf, 0, 512);
    uint32 root_dir_sectors = (512 * 32) / 512;
    uint64 root_start = partition_start_lba + bpb->reserved_sectors + (2 * fat_size_sectors);
    for (uint32 s = 0; s < root_dir_sectors; s++) {
        write_sectors(root_start + s, 1, sector_buf);
    }

    console_print("FAT16 Format Complete.\n");
}
