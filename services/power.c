#include "power.h"

/* Category 10: ACPI power code isolation */

void power_reboot() {
    /* v0: Simple I/O reboot for x86 */
    uint8 good = 0x02;
    while (good & 0x02) {
        /* inb(0x64) */
        __asm__ volatile("inb $0x64, %0" : "=a"(good));
    }
    __asm__ volatile("outb %0, $0x64" : : "a"((uint8)0xFE));
}

void power_shutdown() {
    /* v0: Simple QEMU shutdown or ACPI S5 (not implemented in v0) */
}
