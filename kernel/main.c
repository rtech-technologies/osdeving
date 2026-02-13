#include <efi.h>
#include <efilib.h>
#include "kernel.h"
#include "../services/console.h"
#include "../services/memory.h"
#include "../services/event.h"
#include "../services/disk.h"
#include "../services/fs.h"
#include "../include/system.h"

// --- Kernel Core (Independent of UEFI) ---

#define MAX_SERVICES 32
static service_init_t services[MAX_SERVICES];
static UINTN service_count = 0;
static int running = 1;
static boot_params_t* kernel_params = NULL;

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

void kernel_exit() {
    running = 0;
}

typedef int (*program_main_t)(syscall_table_t*);

#define SHELL_BUFFER_SIZE 65536
#define KERNEL_HEAP_SIZE (4 * 1024 * 1024)

static syscall_table_t global_syscall_table;

void kernel_main(boot_params_t* params) {
    kernel_params = params;

    // 1. Service Registration
    register_service(console_init);
    register_service(memory_init);
    register_service(disk_init);
    register_service(fs_init);

    // 2. Initialization Ritual
    trigger(EVENT_INIT);

    // 3. Setup Syscall Table for Programs
    global_syscall_table.print = print;
    global_syscall_table.alloc = alloc;
    global_syscall_table.free = free;
    global_syscall_table.fread = fread;
    global_syscall_table.fwrite = fwrite;
    global_syscall_table.wait_for_key = wait_for_key;
    global_syscall_table.read_key = read_key;
    global_syscall_table.exit = kernel_exit;

    // 4. Early Diagnostics
    print("Kernel reached main loop (Graphics Mode)\n");

    if (memory_self_test() != 0) {
        print("CRITICAL: Memory self-test failed!\n");
        while(1);
    }

    void* shell_buffer = alloc(SHELL_BUFFER_SIZE);

    // 5. Main Event Loop
    while (running) {
        if (shell_buffer) {
            static int shell_loaded = 0;
            if (!shell_loaded) {
                INTN size = fread("/shell.bin", shell_buffer, SHELL_BUFFER_SIZE);
                if (size > 0) {
                    shell_loaded = 1;
                } else {
                    print("Failed to load shell from disk\n");
                }
            }

            if (shell_loaded) {
                program_main_t shell_main = (program_main_t)shell_buffer;
                shell_main(&global_syscall_table);
                if (running) {
                    print("Program returned. Press any key to reload shell...\n");
                    wait_for_key();
                    shell_loaded = 0;
                }
            } else {
                trigger(EVENT_MAIN);
                for (volatile int i = 0; i < 10000000; i++);
            }
        } else {
            print("Memory allocation for shell failed\n");
            trigger(EVENT_MAIN);
            for (volatile int i = 0; i < 10000000; i++);
        }

        if (running) {
            trigger(EVENT_MAIN);
        }
    }

    // 6. Cleanup Ritual
    trigger(EVENT_CLEANUP);
    trigger(EVENT_EXIT);

    print("Kernel shutdown complete.\n");
}

boot_params_t* get_boot_params() {
    return kernel_params;
}

// --- Bootloader (UEFI Specific) ---

static void* efi_load_file(EFI_HANDLE image, const CHAR16* path, UINTN* out_size) {
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root;
    EFI_FILE_PROTOCOL* file;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_GUID fs_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;

    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, image, &loaded_image_protocol, (void**)&loaded_image);
    if (EFI_ERROR(status)) return NULL;

    status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, loaded_image->DeviceHandle, &fs_protocol, (void**)&fs);
    if (EFI_ERROR(status)) return NULL;

    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root);
    if (EFI_ERROR(status)) return NULL;

    status = uefi_call_wrapper(root->Open, 5, root, &file, (CHAR16*)path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(root->Close, 1, root);
        return NULL;
    }

    UINTN info_size = sizeof(EFI_FILE_INFO) + 256;
    EFI_FILE_INFO* info;
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, info_size, (void**)&info);
    status = uefi_call_wrapper(file->GetInfo, 4, file, &GenericFileInfo, &info_size, info);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(ST->BootServices->FreePool, 1, info);
        uefi_call_wrapper(file->Close, 1, file);
        uefi_call_wrapper(root->Close, 1, root);
        return NULL;
    }

    *out_size = info->FileSize;
    void* buffer;
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, *out_size, &buffer);
    status = uefi_call_wrapper(file->Read, 3, file, out_size, buffer);

    uefi_call_wrapper(ST->BootServices->FreePool, 1, info);
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(root->Close, 1, root);

    return EFI_ERROR(status) ? NULL : buffer;
}

// Removed EFIAPI because gnu-efi crt0 calls efi_main using System V ABI (rdi, rsi)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    // 1. Prepare heap
    void* heap_ptr = NULL;
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, KERNEL_HEAP_SIZE, &heap_ptr);
    if (heap_ptr) {
        memory_set_heap(heap_ptr, KERNEL_HEAP_SIZE);
    }

    // 2. Setup Graphics (GOP)
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    boot_params_t params = {0};

    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->LocateProtocol, 3, &gop_guid, NULL, (void**)&gop);
    if (!EFI_ERROR(status)) {
        params.framebuffer = (uint32*)gop->Mode->FrameBufferBase;
        params.width = gop->Mode->Info->HorizontalResolution;
        params.height = gop->Mode->Info->VerticalResolution;
        params.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    }

    // 3. Load the initial RAM disk content
    UINTN shell_size = 0;
    void* shell_data = efi_load_file(ImageHandle, L"shell.bin", &shell_size);
    if (shell_data) {
        disk_register_file("shell.bin", shell_data, shell_size);
    }

    // 4. Setup Generic Kernel Event System
    event_init();
    register_event_handler(dispatch_init);
    register_event_handler(handle_exit);

    // 5. Handover to Kernel
    kernel_main(&params);

    while(1) {
        __asm__ volatile("hlt");
    }

    return EFI_SUCCESS;
}
