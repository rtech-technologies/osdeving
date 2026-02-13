#ifndef KERNEL_H
#define KERNEL_H

#include "../include/types.h"

typedef void (*service_init_t)();

void register_service(service_init_t init_func);
void stup(boot_params_t* params);
void main();
boot_params_t* get_boot_params();

#endif
