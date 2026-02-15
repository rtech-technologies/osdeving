#include "usb_keyboard.h"
#include "console.h"
#include "input_map.h"
#include "../../unice64/io.h"
#include "../../unice64/kernel.h"

// Standard HID Boot Protocol Report
typedef struct {
    uint8 modifiers;
    uint8 reserved;
    uint8 keycodes[6];
} hid_report_t;

static hid_report_t prev_report = {0};

void usb_keyboard_process_report(const uint8* data, uint32 len) {
    if (len < 8) return;
    hid_report_t* curr = (hid_report_t*)data;

    // Detect new key presses (6-key rollover)
    for (int i = 0; i < 6; i++) {
        uint8 code = curr->keycodes[i];
        if (code == 0) continue;

        // Was it in the previous report?
        int is_new = 1;
        for (int j = 0; j < 6; j++) {
            if (code == prev_report.keycodes[j]) { is_new = 0; break; }
        }

        if (is_new) {
            input_map_push(INPUT_SRC_USB_HID, code, curr->modifiers);
        }
    }

    // Update modifiers if they changed (even if no keys pressed)
    // Here we push a dummy event or just update the internal state of input_map if we had a dedicated modifier setter.
    // For now, input_map_push handles modifiers per key.

    for (int i = 0; i < 8; i++) ((uint8*)&prev_report)[i] = data[i];
}

void usb_keyboard_init(void) {
    // Controller is initialized via init ritual (xhci_init)
}

int usb_has_key(void) {
    xhci_poll();
    return input_map_has_char();
}

int usb_get_key(void) {
    xhci_poll();
    return (int)input_map_pop_char();
}

void usb_poll_all(void) {
    xhci_poll();
}

void usb_lsdev(void) {
    console_print("OSx2 Native USB Stack (xHCI Standard)\n");
    xhci_poll();
}
