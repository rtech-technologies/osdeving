#include "kernel.h"
#include "../services/io/console.h"
#include "../services/io/input.h"
#include "../services/mem/memory.h"
#include "../services/io/disk.h"
#include "../services/io/diskman.h"
#include "../services/fs/fs.h"
#include "../services/fs/fat.h"
#include "../services/fs/rnafs.h"
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
    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}

void kernel_main(boot_params_t* params) {
    kboot_params = *params;

    /* 1. Register Services */
    register_service(console_init);
    register_service(input_init);
    register_service(memory_init);
    register_service(diskman_init);
    register_service(fat_init);
    register_service(fs_init);

    /* 2. Initialize Services */
    for (uint32 i = 0; i < service_count; i++) {
        registered_services[i]();
    }

    trigger(EVENT_INIT);

    print("Kernel started\n");

    /* Populate syscall table */
    syscall_table_t syscalls = {
        .print = print,
        .input = input,
        .fread = fread,
        .fwrite = fwrite,
        .alloc = alloc,
        .free = free,
        .exit = exit_kernel,
        .format = diskman_format_rnafs,
        .mount = diskman_mount_rnafs,
        .lsfs = rnafs_ls,
        .addpart = diskman_add_partition
    };

    /* 3. Load and run shell */
    void* shell_buf = alloc(65536);
    if (shell_buf) {
        if (fread("shell.bin", shell_buf, 65536) > 0) {
            print("Loaded shell.bin\n");
            void (*shell_entry)(boot_params_t*, syscall_table_t*) = (void (*)(boot_params_t*, syscall_table_t*))shell_buf;
            shell_entry(&kboot_params, &syscalls);
        } else {
            print("Failed to load shell.bin\n");
        }
    }

    /* 4. Event Loop */
    while (running) {
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}
