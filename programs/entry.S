.section .text.prologue
.global _start
.extern kernel_entry

_start:
    /* Serial Heartbeat 'A' (Assembly Entry) */
    mov $0x3F8, %dx
    mov $'A', %al
    out %al, %dx

    /* Call C entry */
    jmp kernel_entry
