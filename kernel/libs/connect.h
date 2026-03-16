#ifndef CONNECT_H
#define CONNECT_H

#include "../../include/types.h"

typedef enum {
    DEVICE_TYPE_DISK,
    DEVICE_TYPE_RAM,
    DEVICE_TYPE_USB
} device_type_t;

typedef struct {
    device_type_t type;
    uint32 sector_size;
    uint64 total_lba;
    void*  base_addr; /* For RAM disks */
    int    is_active;
} physical_config_t;

#define MAX_CONNECT_DEVICES 8

void connect_init();
physical_config_t* connect_get_device(const char* path);
void connect_register_device(const char* path, physical_config_t config);

#endif
