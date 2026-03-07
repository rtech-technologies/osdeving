#include "kernel.h"
#include "../libs/console.h"
#include "../libs/kutils.h"
#include "../libs/memory.h"
#include "../libs/input.h"
#include "../libs/disk.h"
#include "../libs/diskman.h"
#include "../libs/fs.h"
#include "../libs/rnafs.h"
#include "../libs/loader.h"
#include "../libs/core/event.h"
#include "../../include/rsl.h"

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

/*
 * The God-Machine Entry Point (Stage 2)
 * Must be at the very top to ensure it's at the start of kernel.bin
 */
void kernel_start(boot_params_t* params) {
    kboot_params = *params;

    register_service(console_init);
    register_service(memory_init);
    register_service(input_init);
    register_service(disk_init);
    register_service(diskman_init);
    register_service(fs_init);
    register_service(loader_init);

    for (uint32 i = 0; i < service_count; i++) {
        registered_services[i]();
    }

    print("OSx2 God-Mode: Kernel Handover Successful.\n");
    trigger(EVENT_INIT);

    rsl_syscall_table_t syscalls = {
        .print = print,
        .input = input,
        .fread = fread,
        .fwrite = fwrite,
        .alloc = alloc,
        .retain = retain,
        .release = release,
        .exit = exit_kernel,
        .clear = console_clear,
        .set_color = console_set_color,
        .format = diskman_format_rnafs,
        .mount = diskman_mount_rnafs,
        .addpart = diskman_add_partition,
        .lsfs = rnafs_ls
    };

    loader_run_shell(&syscalls);

    while (running) {
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}
