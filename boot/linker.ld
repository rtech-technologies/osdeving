ENTRY(kernel_start)
SECTIONS
{
    . = KERNEL_BASE;
    .text : {
        KEEP(*(.text.kernel_start))
        KEEP(*(.text.kernel_start_func))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
