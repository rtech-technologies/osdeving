#include "../include/system.h"
#include "../kernel/kernel.h"

boot_params_t kboot_params;
static int exit_requested = 0;

int program_main();

void exit() {
    exit_requested = 1;
}

void _start(boot_params_t* params) {
    if (params) kboot_params = *params;
    exit_requested = 0;
    program_main();
}
