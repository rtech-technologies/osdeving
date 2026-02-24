#include "kernel.h"
#include "../services/io/console.h"
#include "../services/mem/memory.h"
#include "../services/io/fs.h"
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

    /* 1. Register Services */
    register_service(console_init);
    register_service(memory_init);
    register_service(fs_init);

    /* 2. Initialize Services */
    for (uint32 i = 0; i < service_count; i++) {
        registered_services[i]();
    }

    print("Kernel started\n");

    trigger(EVENT_INIT);

    /* Populate syscall table */
    syscall_table_t syscalls = {
        .print = print,
        .input = input,
        .fread = fread,
        .fwrite = fwrite,
        .alloc = alloc,
        .free = free,
        .exit = exit_kernel
    };

    /* 3. Run shell from ramdisk */
    if (kboot_params.ramdisk_base) {
        void (*shell_entry)(boot_params_t*, syscall_table_t*) = (void (*)(boot_params_t*, syscall_table_t*))kboot_params.ramdisk_base;
        shell_entry(&kboot_params, &syscalls);
    } else {
        print("Error: shell.bin not found in ramdisk\n");
    }

    /* 4. Event Loop */
    while (running) {
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}
