#include "devman.h"
#include "console.h"
#include "memory.h"
#include "pci.h"
#include "input_map.h"
#include "../../unice64/kernel.h"

void devman_init() {
    // Ritual initialization
}

void devman_show() {
    console_print("--- OSx2 Device Manager (devman) ---\n\n");

    // 1. Memory Status
    size_t total, used;
    memory_get_usage(&total, &used);
    console_print("Memory Info:\n");
    console_print("  Total Heap: ");
    // Manual number to string for simplicity
    char buf[32];
    uint64 t = (uint64)total / 1024 / 1024;
    buf[0] = '0' + (t % 10); buf[1] = ' '; buf[2] = 'M'; buf[3] = 'B'; buf[4] = '\n'; buf[5] = 0;
    console_print(buf);

    // 2. Display Status
    boot_params_t* p = get_boot_params();
    console_print("Display Info:\n");
    if (p) {
        console_print("  Resolution: ONLINE (VGA Framebuffer active)\n");
    }

    // 3. Input System
    console_print("Input System Inventory:\n");
    static const char* src_names[] = {"PS/2 Keyboard", "USB HID Keyboard", "Serial Terminal"};
    for (int i = 0; i < INPUT_SRC_MAX; i++) {
        console_print("  ");
        console_print(src_names[i]);
        console_print(": ");
        if (input_map_get_status(i) == INPUT_STATUS_CONNECTED) {
            console_print("ONLINE\n");
        } else {
            console_print("OFFLINE\n");
        }
    }

    // 4. PCI Bus
    pci_ls();

    console_print("\n--- End of Device Report ---\n");
}
