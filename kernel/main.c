#include "kernel.h"
#include "../services/console.h"
#include "../services/memory.h"
#include "../services/input.h"
#include "../services/fs.h"
#include "../services/loader.h"
#include "../services/core/event.h"

boot_params_t kboot_params;
int running = 1;

#define MAX_SERVICES 16
static service_init_t registered_services[MAX_SERVICES];
static uint32 service_count = 0;

void register_service(service_init_t init_func) {
    if (service_count < MAX_SERVICES) {
        registered_services[service_count++] = init_func;
    }
}

void exit_kernel() {
    running = 0;
}

void kernel_main(boot_params_t* params) {
    kboot_params = *params;

    /* Ritual: Register Services */
    register_service(console_init);
    register_service(memory_init);
    register_service(input_init);
    register_service(fs_init);
    register_service(loader_init);

    /* Initialize Services */
    for (uint32 i = 0; i < service_count; i++) {
        registered_services[i]();
    }

    print("Kernel started (Expert Python-like ARC mode)\n");

    trigger(EVENT_INIT);

    /* Populate Syscall Table */
    rsl_syscall_table_t syscalls = {
        .print = print,
        .input = input,
        .fread = fread,
        .fwrite = fwrite,
        .alloc = alloc,
        .retain = retain,
        .release = release,
        .exit = exit_kernel,
        .clear = console_clear
    };

    /* Hand off to loader */
    loader_run_shell(&syscalls);

    /* Event Loop */
    while (running) {
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}
