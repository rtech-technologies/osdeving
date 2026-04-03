ENTRY(_start)
SECTIONS
{
    . = 0x3000000; /* Linked to match Stage 1 load address */

    /* 1. THE UNIFIED BRAIN (All Symbols at Byte 0) */
    .text : {
        *(.text.prologue)
        *(.text*)
    }

    /* 2. THE DATA */
    .data : {
        *(.data*)
    }

    /* 3. THE BSS */
    .bss : {
        *(.bss*)
    }

    /* 4. THE ENGLISH WALL (Moved to the BACK) */
    .rodata : {
        *(.rodata*)
    }
}
