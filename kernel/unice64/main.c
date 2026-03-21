#include "kernel.h"
#include "../../include/rsl.h"
#include "../libs/console.h"
#include "../libs/kutils.h"
#include "../libs/memory.h"
#include "../libs/input.h"
#include "../libs/connect.h"
#include "../libs/vdisk.h"
#include "../libs/disk.h"
#include "../libs/diskman.h"
#include "../libs/fs.h"
#include "../libs/loader.h"
#include "../libs/core/event.h"

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
 * The God-Machine Entry Point
 */
void EFIAPI kernel_main(boot_params_t* params) {
    serial_init();
    if (!params) {
        serial_print("FATAL: EFI passed NULL to Self-Sustaining Kernel\n");
        while(1) { __asm__ volatile("hlt"); }
    }
    kboot_params = *params;

    /* GDT and Interrupts are usually reset after ExitBootServices for full control */
    gdt_init();

    register_service(console_init);
    register_service(memory_init);
    register_service(input_init);
    register_service(connect_init);
    register_service(vdisk_init);
    register_service(disk_init);
    register_service(diskman_init);
    register_service(fs_init);
    register_service(loader_init);

    for (uint32 i = 0; i < service_count; i++) {
        registered_services[i]();
    }

    print("OSx2 God-Mode: Kernel Handover Successful.\n");
    debug_kernel_signature();
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
        .format = (void*)0,
        .mount = (void*)0,
        .addpart = diskman_add_partition,
        .lsfs = (void*)0
    };

    loader_run_shell(&syscalls);

    while (running) {
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}
