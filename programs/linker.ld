ENTRY(_start)
SECTIONS
{
    . = 0x0; /* Raw binary, will be loaded into an allocated buffer */
    .text : {
        KEEP(*(.text._start))
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
