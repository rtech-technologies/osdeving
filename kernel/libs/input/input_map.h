#ifndef INPUT_MAP_H
#define INPUT_MAP_H

#include "types.h"

typedef enum {
    INPUT_SRC_PS2,
    INPUT_SRC_USB_HID,
    INPUT_SRC_SERIAL,
    INPUT_SRC_MAX
} input_source_t;

typedef enum {
    INPUT_STATUS_DISCONNECTED,
    INPUT_STATUS_CONNECTED
} input_device_status_t;

typedef struct {
    input_source_t source;
    uint32 raw_code;
    uint8 modifiers; // For Shift, Alt, Ctrl
} input_event_t;

void input_map_init();
void input_map_push(input_source_t source, uint32 raw_code, uint8 modifiers);
void input_map_set_status(input_source_t source, input_device_status_t status);
input_device_status_t input_map_get_status(input_source_t source);
char input_map_pop_char();
int input_map_has_char();

#endif
