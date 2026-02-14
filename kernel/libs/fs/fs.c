#include "fs.h"
#include "disk.h"
#include "system.h"

void fs_init() {
    // Disk is already initialized
}

INTN fs_read(const char* path, void* buffer, UINTN max_size) {
    return disk_read_file(path, buffer, max_size);
}

INTN fs_write(const char* path, const void* buffer) {
    return -1;
}

// Global API
INTN fread(const char* path, void* buffer, UINTN max_size) {
    return fs_read(path, buffer, max_size);
}

INTN fwrite(const char* path, const void* buffer) {
    return fs_write(path, buffer);
}
