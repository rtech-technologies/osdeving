#include "power.h"

/* Category 10: ACPI power code isolation */

void power_reboot() {
    /* Category 10: outb reboot ritual */
    uint8 data = 0x06;
    __asm__ volatile("outb %0, $0xCF9" : : "a"(data));
}

void power_shutdown() {
    /* ACPI S5 shutdown for QEMU/Bochs */
    uint16 data = 0x2000;
    uint16 port = 0x604;
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));

    /* Fallback for other emulators */
    data = 0x3400;
    port = 0x4004;
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}
