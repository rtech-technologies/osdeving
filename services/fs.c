#include "fs.h"
#include "rnafs.h"

void fs_init() {
    /* RNAFS is initialized via rnafs_mount in kernel_main if needed */
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    /* Category 12: use custom types */
    const char* name = path;
    if (name[0] == '/') name++;
    return (INTN)rnafs_read_file(name, buffer, (uint32)max_size);
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    const char* name = path;
    if (name[0] == '/') name++;
    return (INTN)rnafs_write_file(name, buffer, (uint32)size);
}
