#include "../../include/sys.h"

#define MAX_SERVICES 32
static service_init_t services[MAX_SERVICES];
static UINTN service_count = 0;

int running = 1;
boot_params_t* kparams = NULL;
syscall_table_t ksyscalls;

void register_service(service_init_t init_func) {
    if (service_count < MAX_SERVICES) services[service_count++] = init_func;
}

void dispatch_init(event_t event) {
    if (event == EVENT_INIT) {
        for (UINTN i = 0; i < service_count; i++) services[i]();
    }
}

void handle_exit(event_t event) {
    if (event == EVENT_EXIT) running = 0;
}

void kernel_exit() {
    trigger(EVENT_EXIT);
}

boot_params_t* get_boot_params() { return kparams; }
