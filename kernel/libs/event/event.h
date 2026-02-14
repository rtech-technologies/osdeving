#ifndef EVENT_H
#define EVENT_H

typedef enum {
    EVENT_INIT,
    EVENT_MAIN,
    EVENT_CLEANUP,
    EVENT_EXIT
} event_t;

typedef void (*event_handler_t)(event_t event);

void event_init();
void trigger(event_t event);
void register_event_handler(event_handler_t handler);

#endif
