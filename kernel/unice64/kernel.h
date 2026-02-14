#ifndef KERNEL_H
#define KERNEL_H

#include "../../include/types.h"
#include "../../include/system.h"

typedef void (*service_init_t)();

void register_service(service_init_t init_func);
void stup(boot_params_t* params);
void main();
boot_params_t* get_boot_params();
void kernel_exit();

extern int running;
extern syscall_table_t ksyscalls;
extern boot_params_t* kparams;

// Event handlers
void dispatch_init(event_t event);
void handle_exit(event_t event);

#endif
