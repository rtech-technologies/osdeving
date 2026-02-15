#include "pci.h"
#include "console.h"
#include "../../unice64/io.h"

#define MAX_PCI_DEVICES 64
static pci_device_t pci_inventory[MAX_PCI_DEVICES];
static int pci_count = 0;

const char* pci_class_to_str(uint8 class_code) {
    switch (class_code) {
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        default:   return "Unknown Device Class";
    }
}

void pci_init() {
    pci_count = 0;
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            for (int func = 0; func < 8; func++) {
                uint32 v = pci_read_config_32(bus, slot, func, 0);
                uint16 vendor = v & 0xFFFF;
                if (vendor == 0xFFFF) continue;

                if (pci_count < MAX_PCI_DEVICES) {
                    pci_inventory[pci_count].bus = (uint8)bus;
                    pci_inventory[pci_count].slot = (uint8)slot;
                    pci_inventory[pci_count].func = (uint8)func;
                    pci_inventory[pci_count].vendor_id = vendor;
                    pci_inventory[pci_count].device_id = (v >> 16) & 0xFFFF;

                    uint32 c = pci_read_config_32(bus, slot, func, 0x08);
                    pci_inventory[pci_count].class_code = (c >> 24) & 0xFF;
                    pci_inventory[pci_count].sub_class = (c >> 16) & 0xFF;

                    pci_count++;
                }

                // If it's not a multi-function device, don't check other functions
                if (func == 0) {
                    uint32 h = pci_read_config_32(bus, slot, 0, 0x0C);
                    if (!((h >> 16) & 0x80)) break;
                }
            }
        }
    }
}

void pci_ls() {
    console_print("--- PCI Hardware Inventory ---\n");
    for (int i = 0; i < pci_count; i++) {
        console_print(pci_class_to_str(pci_inventory[i].class_code));
        console_print(" (");
        // Simplified ID print
        if (pci_inventory[i].vendor_id == 0x8086) console_print("Intel");
        else if (pci_inventory[i].vendor_id == 0x1234) console_print("QEMU");
        else console_print("Other");
        console_print(")\n");
    }
}

int pci_get_devices(pci_device_t* list, int max) {
    int count = (pci_count < max) ? pci_count : max;
    for (int i = 0; i < count; i++) {
        list[i] = pci_inventory[i];
    }
    return count;
}
