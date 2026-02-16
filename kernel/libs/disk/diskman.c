#include "diskman.h"
#include "console.h"
#include "../fs/rnafs.h"

typedef struct {
    uint64 start_lba;
    uint32 sector_count;
    int type; // 1 for RNAFS
} partition_info_t;

#define MAX_PARTITIONS 4
static partition_info_t partitions[MAX_PARTITIONS];
static int partition_count = 0;

void diskman_init() {
    // Mock partition discovery
    // Assume a 4MB partition starts at 1MB (2048 sectors)
    partitions[0].start_lba = 2048;
    partitions[0].sector_count = 8192; // 4 MB
    partitions[0].type = 1;
    partition_count = 1;
}

void diskman_format() {
    if (partition_count > 0) {
        mkfs_rnafs(partitions[0].start_lba, partitions[0].sector_count);
    }
}

void diskman_mount() {
    if (partition_count > 0) {
        rnafs_mount(partitions[0].start_lba);
    }
}

void diskman_ls() {
    rnafs_ls();
}
