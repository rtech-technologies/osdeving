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

    for (int i = 0; i < 8; i++) ((uint8*)&prev_report)[i] = data[i];
}

void usb_keyboard_init(void) {
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

static const char* src_names[] = {"PS/2 Keyboard", "USB HID Keyboard", "Serial Terminal"};

void usb_lsdev(void) {
    console_print("--- OSx2 Input System Inventory ---\n");
    for (int i = 0; i < INPUT_SRC_MAX; i++) {
        console_print(src_names[i]);
        console_print(": ");
        if (input_map_get_status(i) == INPUT_STATUS_CONNECTED) {
            console_print("ONLINE\n");
        } else {
            console_print("NOT DETECTED\n");
        }
    }
}
