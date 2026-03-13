ENTRY(_start)
SECTIONS
{
    . = KERNEL_BASE;
    .text : {
        KEEP(*(.text.kernel_start))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
