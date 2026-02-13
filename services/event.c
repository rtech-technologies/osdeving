#include "event.h"
#include "../include/types.h"
#include "../include/system.h"

#define MAX_HANDLERS 64

static event_handler_t handlers[MAX_HANDLERS];
static int handler_count = 0;

void event_init() {
    handler_count = 0;
}

void register_event_handler(event_handler_t handler) {
    if (handler_count < MAX_HANDLERS) {
        handlers[handler_count++] = handler;
    }
}

void trigger(event_t event) {
    for (int i = 0; i < handler_count; i++) {
        handlers[i](event);
    }
}

void exit() {
    trigger(EVENT_EXIT);
}
