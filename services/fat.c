#include "fat.h"
#include "../include/utils.h"

/* FAT service stub. Will provide FAT access to block devices / USB.
   For now this is a placeholder that returns not-implemented. */

void fat_init() {
    /* Initialize FAT drivers / mount points here in the future */
}

INTN fat_read(const char* path, void* buffer, uint64 max_size) {
    (void)path; (void)buffer; (void)max_size;
    return -1; /* not implemented */
}
