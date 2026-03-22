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

/* Syscall Wrappers */
static void sys_mount(int idx) {
    char name[16];
    strcpy(name, "VDISK");
    char num[4];
    itoa(idx, num, 10);
    strcat(name, num);

    vdisk_t* vd = vdisk_open(name);
    if (vd) {
        vdisk_mount_verify(vd);
    }
}

static void sys_format(int idx) {
    print("Format: FAT32 auto-format not yet implemented in Opaque Mode.\n");
}

static void sys_lsfs() {
    print("FS: FAT32 directory listing via VFS.\n");
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
        print("Kernel: Initializing service...\n");
        registered_services[i]();
    }

    print("OSx2 God-Mode: Kernel Handover Successful.\n");
    serial_print("Kernel: Handover complete. Triggering initialization events.\n");
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
        .format = sys_format,
        .mount = sys_mount,
        .addpart = diskman_add_partition,
        .lsfs = sys_lsfs
    };

    loader_run_shell(&syscalls);

    while (running) {
        trigger(EVENT_MAIN);
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}
