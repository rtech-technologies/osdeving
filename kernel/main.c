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
static UINTN service_count = 0;

static int running = 1;

void register_service(service_init_t init_func) {
    if (service_count < MAX_SERVICES) {
        services[service_count++] = init_func;
    }
}

static void dispatch_init(event_t event) {
    if (event == EVENT_INIT) {
        for (UINTN i = 0; i < service_count; i++) {
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

#define SHELL_BUFFER_SIZE 65536

void kernel_main() {
    // Service Registration
    register_service(console_init);
    register_service(memory_init);
    register_service(disk_init);
    register_service(fs_init);

    // Initial trigger
    trigger(EVENT_INIT);

    print("Kernel reached main loop\n");

    void* shell_buffer = alloc(SHELL_BUFFER_SIZE);

    // Main Event Loop
    while (running) {
        if (shell_buffer) {
            INTN size = fread("/shell.bin", shell_buffer, SHELL_BUFFER_SIZE);
            if (size > 0) {
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

    // Cleanup and Exit
    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}

// efi_main called by gnu-efi crt0.o (standard SysV ABI on Linux)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    disk_init_with_handle(ImageHandle);
    event_init();

    register_event_handler(dispatch_init);
    register_event_handler(handle_exit);

    kernel_main();

    // Prevent return to firmware
    while(1);

    return EFI_SUCCESS;
}
