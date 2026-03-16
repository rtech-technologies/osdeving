ENTRY(_start)
SECTIONS
{
    . = 0x3000000; /* Linked to match Stage 1 load address */
    .text : {
        KEEP(*(.text._start))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
