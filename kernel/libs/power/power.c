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
    // 1. Keyboard controller reset (32-bit safe)
    unsigned int kbc_cmd = 0xFE;
    __asm__ volatile ("outl %0, %1" : : "a"(kbc_cmd), "Nd"(0x64));

    // 2. Short delay
    for(volatile int i = 0; i < 100000; i++);

    // 3. Legacy system reset port (0x92)
    unsigned int sys_reset = 0x06;
    __asm__ volatile ("outl %0, %1" : : "a"(sys_reset), "Nd"(0x92));

    // 4. Final fallback: halt CPU
    while(1) { __asm__ volatile("hlt"); }
}

// Hardware-level shutdown using outl
void shutdown() {
    console_print("System Shutting Down...\n");
    // ACPI S5 soft-off
    unsigned int value = 0x2000;   // S5 sleep + sleep enable
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(0x604)); // PM1a control

    // Optional PM1b (if motherboard present)
    // __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(0x608));

    // Small delay for motherboard to respond
    for(volatile int i = 0; i < 100000; i++);

    // Fallback: halt CPU if ACPI fails
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
