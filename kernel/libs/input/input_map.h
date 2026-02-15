#ifndef INPUT_MAP_H
#define INPUT_MAP_H

#include "types.h"

typedef enum {
    INPUT_SRC_PS2,
    INPUT_SRC_USB_HID,
    INPUT_SRC_SERIAL,
    INPUT_SRC_MAX
} input_source_t;

typedef struct {
    input_source_t source;
    uint32 raw_code;
    uint8 modifiers; // For Shift, Alt, Ctrl
} input_event_t;

void input_map_init();
void input_map_push(input_source_t source, uint32 raw_code, uint8 modifiers);
char input_map_pop_char();
int input_map_has_char();

#endif
