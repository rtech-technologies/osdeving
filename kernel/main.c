#include <efi.h>
#include <efilib.h>
#include "kernel.h"
#include "../services/console.h"
#include "../services/memory.h"
#include "../services/event.h"
#include "../services/disk.h"
#include "../services/fs.h"
#include "../include/system.h"

// Service registry
#define MAX_SERVICES 32
static service_init_t services[MAX_SERVICES];
static int service_count = 0;

static int running = 1;

void register_service(service_init_t init_func) {
    if (service_count < MAX_SERVICES) {
        services[service_count++] = init_func;
    }
}

static void dispatch_init(event_t event) {
    if (event == EVENT_INIT) {
        for (int i = 0; i < service_count; i++) {
            services[i]();
        }
    }
}

static void handle_exit(event_t event) {
    if (event == EVENT_EXIT) {
        running = 0;
    }
}

typedef int (*program_main_t)(EFI_SYSTEM_TABLE* st);

void kernel_main() {
    register_service(console_init);
    register_service(memory_init);
    register_service(disk_init);
    register_service(fs_init);

    trigger(EVENT_INIT);

    print("Kernel started\n");

    void* shell_buffer = alloc(65536);

    while (running) {
        if (shell_buffer) {
            print("Loading /shell.bin...\n");
            int size = fread("/shell.bin", shell_buffer);
            if (size > 0) {
                print("Running shell...\n");
                program_main_t shell_main = (program_main_t)shell_buffer;
                shell_main(ST);
                if (running) {
                    print("Program returned\n");
                }
            } else {
                print("Failed to load shell\n");
                trigger(EVENT_MAIN);
            }
        } else {
            print("Memory allocation for shell failed\n");
            trigger(EVENT_MAIN);
        }

        if (running) {
            trigger(EVENT_MAIN);
        }
    }

    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}

// efi_main is called by crt0.o which handles the MS ABI to SysV ABI conversion.
EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* st) {
    InitializeLib(image, st);

    disk_init_with_handle(image);
    event_init();

    register_event_handler(dispatch_init);
    register_event_handler(handle_exit);

    console_init();
    memory_init();

    kernel_main();

    return EFI_SUCCESS;
}
