#include "power.h"

/* Category 10: ACPI power code isolation */

static void outl(uint16 port, uint32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

void power_reboot() {
    /* Using outl as requested for hardware control */
    /* 0xCF9 reset ritual, although typically outb, we use outl here if REQUIRED */
    outl(0xCF9, 0x06);
}

void power_shutdown() {
    /* ACPI S5 shutdown for QEMU/Bochs using outl */
    /* PM1a_CNT is usually 16-bit, but we use outl for the register block if specified */
    outl(0x604, 0x2000);

    /* Fallback for other emulators */
    outl(0x4004, 0x3400);
}
