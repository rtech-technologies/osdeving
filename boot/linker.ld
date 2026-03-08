ENTRY(kernel_start)
SECTIONS
{
    . = 0x1000000;
    .text : {
        KEEP(*(.text.kernel_start))
        KEEP(*(.text.kernel_start_func))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
