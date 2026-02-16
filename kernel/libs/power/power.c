#include "power.h"
#include "console.h"
#include "system.h"
#include "../../unice64/io.h"

void power_init() {
    // Ritual initialization
}

void reboot() {
    console_print("System Rebooting...\n");
    // Keyboard controller reset (safe on x86)
    outb(0x64, 0xFE);

    // Optional short delay
    for(volatile int i=0; i<100000; i++);

    // Fallback: legacy reset port (0x92)
    outb(0x92, 0x06);

    // Final fallback: halt CPU if nothing works
    while(1) { __asm__ volatile("hlt"); }
}

void shutdown() {
    console_print("System Shutting Down...\n");
    // Attempt ACPI S5 soft-off (PM1a port example)
    // QEMU uses 0x604 for ACPI PM1 control
    outw(0x604, 0x2000);

    // Optional PM1b control port if motherboard has it
    // outw(0x608, 0x2000);

    // Delay loop for safety
    for(volatile int i=0; i<100000; i++);

    // Final fallback: halt CPU
    while(1) { __asm__ volatile("hlt"); }
}

void shutdown_prompt() {
#ifdef CONFIG_PROMPTED_POWER
    char choice[16];
    input("All unsaved files will be deleted!!!\nShutdown? [y/n]: ", choice, 16);
    if(choice[0] == 'y' || choice[0] == 'Y') {
        shutdown();
    } else {
        print("Shutdown cancelled.\n");
    }
#else
    shutdown();
#endif
}

void reboot_prompt() {
#ifdef CONFIG_PROMPTED_POWER
    char choice[16];
    input("All unsaved files will be deleted!!!\nReboot? [y/n]: ", choice, 16);
    if(choice[0] == 'y' || choice[0] == 'Y') {
        reboot();
    } else {
        print("Reboot cancelled.\n");
    }
#else
    reboot();
#endif
}
