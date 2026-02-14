#include "usb_keyboard.h"
#include "console.h"
#include "../../unice64/io.h"

#define USB_KEY_BUFFER_SIZE 16

static uint8 key_buffer[USB_KEY_BUFFER_SIZE];
static int buffer_start = 0;
static int buffer_end = 0;

uint8 translate_hid_to_keycode(uint8 hid_code) {
    if (hid_code >= 0x04 && hid_code <= 0x1D) return 'a' + (hid_code - 0x04);
    if (hid_code == 0x28) return '\n';
    if (hid_code == 0x2C) return ' ';
    return 0;
}

int usb_read_interrupt(uint8* report) {
    return 0;
}

void usb_handle_irq_c(void) {
    uint8 report[8];
    if (usb_read_interrupt(report)) {
        key_buffer[buffer_end] = translate_hid_to_keycode(report[2]);
        buffer_end = (buffer_end + 1) % USB_KEY_BUFFER_SIZE;
    }
}

void usb_init_controller(void) {
    console_print("Scanning for USB Controllers...\n");
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            uint32 val = pci_read_config_32(bus, dev, 0, 0);
            if ((val & 0xFFFF) == 0xFFFF) continue;

            uint32 class_rev = pci_read_config_32(bus, dev, 0, 0x08);
            uint8 base_class = (class_rev >> 24) & 0xFF;
            uint8 sub_class = (class_rev >> 16) & 0xFF;
            uint8 prog_if = (class_rev >> 8) & 0xFF;

            if (base_class == 0x0C && sub_class == 0x03) {
                const char* type = "Unknown";
                if (prog_if == 0x00) type = "UHCI";
                else if (prog_if == 0x10) type = "OHCI";
                else if (prog_if == 0x20) type = "EHCI";
                else if (prog_if == 0x30) type = "XHCI";

                console_print("Found USB Controller: ");
                console_print(type);
                console_print("\n");
            }
        }
    }
}

void usb_enumerate_hid_keyboard(void) {
    // Placeholder for actual enumeration
}

int usb_get_key(void) {
    if (buffer_start == buffer_end) return 0;
    int key = key_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % USB_KEY_BUFFER_SIZE;
    return key;
}

void usb_keyboard_init(void) {
    usb_init_controller();
    usb_enumerate_hid_keyboard();
}
