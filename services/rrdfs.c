#include "rrdfs.h"
#include "rnafs.h"
#include "../include/utils.h"

/* RRDFS: wrapper for existing RNAFS implementation for now.
   Later this will be a full replacement providing better large-file handling. */

void rrdfs_init() {
    /* currently delegate to rnafs_init to avoid breaking existing behavior */
    rnafs_init();
}

INTN rrdfs_read(const char* path, void* buffer, uint64 max_size) {
    return rnafs_read(path, buffer, max_size);
}

INTN rrdfs_write(const char* path, const void* buffer, uint64 size) {
    return rnafs_write(path, buffer, size);
}
