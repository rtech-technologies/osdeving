#include "connect.h"
#include "kutils.h"
#include "../unice64/kernel.h"

typedef struct {
    char path[32];
    physical_config_t config;
} connect_entry_t;

static connect_entry_t registry[MAX_CONNECT_DEVICES];
static uint32 device_count = 0;

void connect_init() {
    device_count = 0;
    memset(registry, 0, sizeof(registry));

    /* Register System RAM Disk */
    physical_config_t ram0;
    ram0.type = DEVICE_TYPE_RAM;
    ram0.sector_size = 512;
    ram0.total_lba = kboot_params.ramdisk_size / 512;
    ram0.base_addr = kboot_params.ramdisk_base;
    ram0.is_active = 1;
    connect_register_device("/CONNECT/RAM0/", ram0);

    /* Register Placeholder USB */
    physical_config_t usb0;
    usb0.type = DEVICE_TYPE_USB;
    usb0.sector_size = 512;
    usb0.total_lba = 0;
    usb0.base_addr = (void*)0;
    usb0.is_active = 0; /* Not connected yet */
    connect_register_device("/CONNECT/USB0/", usb0);
}

void connect_register_device(const char* path, physical_config_t config) {
    if (device_count < MAX_CONNECT_DEVICES) {
        strcpy(registry[device_count].path, path);
        registry[device_count].config = config;
        device_count++;
    }
}

physical_config_t* connect_get_device(const char* path) {
    for (uint32 i = 0; i < device_count; i++) {
        if (strcmp(registry[i].path, path) == 0) {
            return &registry[i].config;
        }
    }
    return (void*)0;
}
