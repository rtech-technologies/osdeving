#include "fs.h"
#include "../disk/disk.h"
#include "rnafs.h"

void fs_init() {
    // Initialized by registry
}

INTN fs_read(const char* path, void* buffer, UINTN max_size) {
    // 1. Check Boot RamDisk (pre-loaded files)
    INTN ret = disk_read_file(path, buffer, max_size);
    if (ret >= 0) return ret;

    // 2. Check RNAFS
    const char* name = path;
    if (name[0] == '/') name++;
    ret = (INTN)rnafs_read_file(name, buffer, (uint32)max_size);
    if (ret > 0) return ret;

    return -1;
}

INTN fs_delete(const char* path) {
    const char* name = path;
    if (name[0] == '/') name++;

    if (rnafs_delete_file(name)) return 0;
    return -1;
}

INTN fs_write(const char* path, const void* buffer) {
    // In v0, we only support writing to RNAFS
    // We assume the caller provides the full buffer and we overwrite.
    // For simplicity, we assume we know the size...
    // Wait, fs_write doesn't have a size parameter in the current API.
    // I should probably add one or use a default.
    // The shell might need to pass size.
    return -1;
}

// Extended API for internal use
INTN fs_write_sized(const char* path, const void* buffer, UINTN size) {
    const char* name = path;
    if (name[0] == '/') name++;

    // Try update existing
    INTN ret = (INTN)rnafs_write_file(name, buffer, (uint32)size);
    if (ret > 0) return ret;

    // Try create new
    if (rnafs_create_file(0, name, (uint32)size)) {
        return (INTN)rnafs_write_file(name, buffer, (uint32)size);
    }

    return -1;
}
