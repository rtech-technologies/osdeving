#include "kernel.h"
#include "../services/console.h"
#include "../services/memory.h"
#include "../services/disk.h"
#include "../services/fs.h"
#include "../services/event.h"

EFI_SYSTEM_TABLE *ST_PTR;
EFI_HANDLE ImageHandle_PTR;
int running = 1;

#define MAX_SERVICES 16
static service_init_t registered_services[MAX_SERVICES];
static UINTN service_count = 0;

void register_service(service_init_t init_func) {
    if (service_count < MAX_SERVICES) {
        registered_services[service_count++] = init_func;
    }
}

void on_init(event_t event) {
    if (event == EVENT_INIT) {
        for (UINTN i = 0; i < service_count; i++) {
            registered_services[i]();
        }
    }
}

void exit() {
    running = 0;
    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);
}

void kernel_main() {
    // 1. Setup Event System
    event_init();
    register_event_handler(on_init);

    // 2. Register Services
    register_service(console_init);
    register_service(memory_init);
    register_service(disk_init);
    register_service(fs_init);

    // 3. Trigger INIT event
    trigger(EVENT_INIT);

    print("Kernel started\n");

    // 4. Load Shell
    void* shell_buf = alloc(65536);
    if (shell_buf) {
        INTN sz = fread("shell.bin", shell_buf, 65536);
        if (sz > 0) {
            print("Loaded shell.bin\n");
            void (*shell_entry)(EFI_SYSTEM_TABLE*) = (void (*)(EFI_SYSTEM_TABLE*))shell_buf;
            shell_entry(ST_PTR);
        } else {
            print("Failed to load shell.bin\n");
        }
    }

    // 5. Main Loop
    while (running) {
        trigger(EVENT_MAIN);
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST_PTR = SystemTable;
    ImageHandle_PTR = ImageHandle;

    kernel_main();

    return EFI_SUCCESS;
}
