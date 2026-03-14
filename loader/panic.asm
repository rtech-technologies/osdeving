[BITS 64]
global capture_registers
extern loader_panic_handler

section .text
capture_registers:
    ; Input: RDI = Error Message (CHAR16*), RSI = Status (EFI_STATUS)
    ; We need to save all registers into a struct and pass it to C

    ; Setup a stack frame
    push rbp
    mov rbp, rsp

    ; Save all general purpose registers
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi ; Error message
    push rsi ; Status
    push rbp ; Saved RBP
    push rdx
    push rcx
    push rbx
    push rax

    ; Current RSP is at the top of our saved registers
    mov rdx, rsp ; Argument 3: register_state_t*
    ; RDI already contains Message (Arg 1)
    ; RSI already contains Status (Arg 2)

    call loader_panic_handler

    ; Should never return
    add rsp, 120
    pop rbp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
