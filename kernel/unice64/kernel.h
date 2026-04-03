#ifndef KERNEL_H
#define KERNEL_H

#include "../../include/types.h"
#include "../../include/rsl.h"
#include "../../include/config.h"

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

/* Entry point defined in main.c */
void EFIAPI kernel_main(boot_params_t* params);

/* GDT initialization */
void gdt_init(void);

#endif
