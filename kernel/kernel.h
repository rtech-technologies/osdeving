#ifndef KERNEL_H
#define KERNEL_H

#include "../include/types.h"

typedef void (*service_init_t)();

void register_service(service_init_t init_func);
void kernel_main(boot_params_t* params);
boot_params_t* get_boot_params();

#endif
