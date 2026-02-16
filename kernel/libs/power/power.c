#include "power.h"
#include "console.h"
#include "system.h"
#include "../../unice64/io.h"

void power_init() {
    // Ritual initialization
}

// ==========================
// Basic Kernel Shutdown & Reboot (using outl)
// ==========================

// Hardware-level reboot
void reboot() {
    console_print("System Rebooting...\n");
    // 1. Keyboard controller reset (safe)
    outb(0x64, 0xFE);

    // 2. Short delay
    for(volatile int i=0; i<100000; i++);

    // 3. Legacy system reset port (0x92)
    outb(0x92, 0x06);

    // 4. Final fallback: halt CPU
    while(1) { __asm__ volatile("hlt"); }
}

// Hardware-level shutdown using outl
void shutdown() {
    console_print("System Shutting Down...\n");
    // ACPI S5 soft-off
    unsigned int value = 0x2000;   // 16-bit S5 + sleep enable, zero-extended to 32-bit
    outl(0x604, value);            // PM1a control port

    // Optional PM1b (some motherboards)
    // outl(0x608, value);          // PM1b control port

    // Small delay to allow motherboard to respond
    for(volatile int i=0; i<100000; i++);

    // Fallback: halt CPU if motherboard doesn't respond
    while(1) { __asm__ volatile("hlt"); }
}

// ==========================
// User Prompts
// ==========================

// Prompted shutdown
void shutdown_prompt() {
    char choice[16];
    input("All unsaved files will be deleted!!!\nShutdown? [y/n]: ", choice, 16);
    if(choice[0] == 'y' || choice[0] == 'Y') {
        shutdown();
    } else {
        print("Shutdown cancelled.\n");
    }
}

// Prompted reboot
void reboot_prompt() {
    char choice[16];
    input("All unsaved files will be deleted!!!\nReboot? [y/n]: ", choice, 16);
    if(choice[0] == 'y' || choice[0] == 'Y') {
        reboot();
    } else {
        print("Reboot cancelled.\n");
    }
}

// ==========================
// Shell Command Integration
// ==========================
void shell_shutdown() { shutdown_prompt(); }
void shell_reboot()   { reboot_prompt(); }
