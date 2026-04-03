#include "fs.h"
#include "console.h"

/* VFS Placeholder for FAT-only implementation */

void fs_init() {
    print("VFS: FAT-only mode initialized.\n");
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    /* TODO: Implement FAT32 read via VDISK */
    return -1;
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    /* TODO: Implement FAT32 write via VDISK */
    return -1;
}
