#ifndef KERNEL_H
#define KERNEL_H

#include "../include/types.h"
#include "../include/system.h"
#include "../boot/efi_types.h"

/* Category 11: Framebuffer params with UEFI */
typedef struct {
    uint32* framebuffer;
    uint32  width;
    uint32  height;
    uint32  pixels_per_scanline;

    /* Ramdisk info for v0 disk model */
    void*   ramdisk_base;
    uint64  ramdisk_size;

    /* UEFI handles */
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE       ImageHandle;
} boot_params_t;

/* Events */
typedef enum {
    EVENT_INIT,
    EVENT_MAIN,
    EVENT_CLEANUP,
    EVENT_EXIT
} event_t;

/* Service Registry */
typedef void (*service_init_t)();
void register_service(service_init_t init_func);

/* Event System */
void trigger(event_t event);
typedef void (*event_handler_t)(event_t event);
void register_event_handler(event_handler_t handler);

/* Global State */
extern int running;
extern boot_params_t kboot_params;

void kernel_main(boot_params_t* params);

#endif
