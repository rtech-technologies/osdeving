.section .text.prologue
.global _start
.extern kernel_entry

_start:
    /*
     * Handover from OSx2 Kernel (System V ABI):
     * RDI = boot_params_t*
     * RSI = rsl_syscall_table_t*
     */

    /* Serial Heartbeat 'A' (Assembly Entry) */
    /* Preserve RDI/RSI across IO operation */
    push %rdi
    push %rsi

    mov $0x3F8, %dx
    mov $'A', %al
    out %al, %dx

    pop %rsi
    pop %rdi

    /*
     * Already in System V ABI:
     * RDI = arg0 (boot_params_t*)
     * RSI = arg1 (rsl_syscall_table_t*)
     */

    /* Call C entry */
    jmp kernel_entry
