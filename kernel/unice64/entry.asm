[BITS 64]
global _start
extern kernel_main

section .text
_start:
    ; The Stage 1 loader passes boot_params_t* in RDI
    ; 1. Set up a fresh stack (16KB)
    mov rsp, stack_top

    ; 2. Call your C kernel (passing RDI along)
    call kernel_main

    ; 3. If the kernel ever returns, just halt
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384 ; 16KB stack
stack_top:
