#include "loader.h"
#include "console.h"
#include "../unice64/kernel.h"

void loader_init() {
    /* No-op init */
}

void loader_run_shell(rsl_syscall_table_t* syscalls) {
    if (kboot_params.ramdisk_base) {
        void (*shell_entry)(boot_params_t*, rsl_syscall_table_t*) =
            (void (*)(boot_params_t*, rsl_syscall_table_t*))kboot_params.ramdisk_base;
        shell_entry(&kboot_params, syscalls);
    } else {
        print("Error: shell.bin not found in ramdisk\n");
    }
}
