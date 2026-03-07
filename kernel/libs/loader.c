#include "loader.h"
#include "console.h"
#include "fs.h"
#include "memory.h"
#include "../unice64/kernel.h"

void loader_init() {
}

void loader_run_shell(rsl_syscall_table_t* syscalls) {
    #ifdef CONFIG_LOAD_SHELL
    /* Stage 2 Loader: Load shell.bin from disk using kernel drivers */
    char* shell_buf = (char*)alloc(65536);
    if (shell_buf) {
        /* Use fread (kernel-side) instead of read_file (RSL-side) */
        if (fread("shell.bin", shell_buf, 65536) > 0) {
             void (*shell_entry)(boot_params_t*, rsl_syscall_table_t*) =
                (void (*)(boot_params_t*, rsl_syscall_table_t*))shell_buf;
             shell_entry(&kboot_params, syscalls);
        } else {
             print("Loader: shell.bin not found on disk. (Is RNAFS mounted?)\n");
        }
    }
    #else
    print("Loader: Auto-load shell disabled by config.\n");
    #endif
}
