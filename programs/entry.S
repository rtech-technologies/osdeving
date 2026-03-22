.section .text.prologue
.global _start
.extern kernel_entry

_start:
    /*
     * Handover from UEFI (Microsoft x64 ABI):
     * RCX = boot_params_t*
     * RDX = rsl_syscall_table_t*
     */

    /* Serial Heartbeat 'A' (Assembly Entry) */
    /* Preserve RCX/RDX across IO operation */
    push %rcx
    push %rdx

    mov $0x3F8, %dx
    mov $'A', %al
    out %al, %dx

    pop %rdx
    pop %rcx

    /*
     * Translate to System V ABI (for programs/kernel_entry):
     * RDI = arg0 (boot_params_t*)
     * RSI = arg1 (rsl_syscall_table_t*)
     */
    mov %rcx, %rdi
    mov %rdx, %rsi

    /* Call C entry */
    jmp kernel_entry
