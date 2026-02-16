#include "../include/system.h"
#include "../kernel/kernel.h"

boot_params_t kboot_params;

int program_main();

void exit() {
    /* v0: Just spin or return if possible */
    while(1);
}

void _start(boot_params_t* params) {
    if (params) kboot_params = *params;
    program_main();
}
