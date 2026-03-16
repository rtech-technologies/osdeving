#ifndef VDISK_H
#define VDISK_H

#include "../../include/types.h"
#include "connect.h"

typedef struct {
    char name[32];
    physical_config_t* physical;
    uint64 start_lba;
    uint64 end_lba;
    int is_mounted;
} vdisk_t;

#define MAX_VDISKS 8

void vdisk_init();
vdisk_t* vdisk_open(const char* name);
int vdisk_read(vdisk_t* vd, uint64 lba, uint32 count, void* buffer);
int vdisk_write(vdisk_t* vd, uint64 lba, uint32 count, const void* buffer);
int vdisk_mount_verify(vdisk_t* vd);

#endif
