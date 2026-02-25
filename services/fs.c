#include "fs.h"
#include "rrdfs.h"

void fs_init() {
    rrdfs_init();
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    return rrdfs_read(path, buffer, max_size);
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    return rrdfs_write(path, buffer, size);
}
