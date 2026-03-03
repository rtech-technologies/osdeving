#include "fs.h"
#include "rnafs.h"

void fs_init() {
    rnafs_init();
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    return rnafs_read(path, buffer, max_size);
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    return rnafs_write(path, buffer, size);
}
