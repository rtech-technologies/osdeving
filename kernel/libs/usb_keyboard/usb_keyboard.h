#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include "types.h"

void usb_keyboard_init(void);
int usb_get_key(void);
int usb_has_key(void);

// Internal/Placeholder functions from user spec
void usb_init_controller(void);
void usb_enumerate_hid_keyboard(void);
int usb_read_interrupt(uint8* report);
uint8 translate_hid_to_keycode(uint8 hid_code);

#endif
