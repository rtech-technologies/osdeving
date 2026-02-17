#include "power.h"

/* Category 10: ACPI power code isolation */

static void outl(uint16 port, uint32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "d"(port));
}

void power_reboot() {
    /* Port 0xCF9: Reset Control Register */
    /* Value 0x06: System Reset + Reset CPU */
    outl(0xCF9, 0x06);
}

void power_shutdown() {
    /* ACPI S5 shutdown for QEMU/Bochs using outl */
    outl(0x604, 0x2000);

    /* Fallback for other emulators */
    outl(0x4004, 0x3400);
}
