#include "../boot/efi_types.h"
#include "../include/rsl.h"

static EFI_GUID li_g = {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID fs_g = {0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID gop_g = {0x70482061, 0x0512, 0x476A, {0xBC, 0xC3, 0x04, 0x54, 0x8F, 0x9E, 0x22, 0x07}};
static EFI_GUID info_g = {0x0964E5B2, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

static void print_hex(EFI_SYSTEM_TABLE *SystemTable, uint64 val) {
    CHAR16 buf[19];
    buf[0] = L'0'; buf[1] = L'x';
    for (int i = 0; i < 16; i++) {
        uint8 v = (val >> ((15 - i) * 4)) & 0xF;
        buf[i+2] = (v < 10) ? (L'0' + v) : (L'A' + v - 10);
    }
    buf[18] = 0;
    SystemTable->ConOut->OutputString(SystemTable->ConOut, buf);
}

static EFI_STATUS load_file(EFI_SYSTEM_TABLE *ST, EFI_FILE_PROTOCOL *root, CHAR16 *name, EFI_PHYSICAL_ADDRESS addr, uint64 *out_size) {
    EFI_FILE_PROTOCOL *file;
    EFI_STATUS status = root->Open(root, &file, name, 0x0000000000000001ULL, 0);
    if (status != EFI_SUCCESS) return status;

    uint8 info_buffer[256];
    UINTN info_size = sizeof(info_buffer);
    status = file->GetInfo(file, &info_g, &info_size, info_buffer);
    if (status != EFI_SUCCESS) return status;

    uint64 file_size = ((EFI_FILE_INFO*)info_buffer)->FileSize;
    UINTN pages = (file_size + 4095) / 4096;
    EFI_PHYSICAL_ADDRESS load_addr = addr;

    status = ST->BootServices->AllocatePages(2, 2, pages, &load_addr);
    if (status != EFI_SUCCESS) return status;

    UINTN read_size = (UINTN)file_size;
    status = file->Read(file, &read_size, (void*)load_addr);
    if (status == EFI_SUCCESS && out_size) *out_size = (uint64)read_size;

    file->Close(file);
    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    boot_params_t params = {0};
    EFI_STATUS status;

    /* 1. Get Graphics Info */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = SystemTable->BootServices->LocateProtocol(&gop_g, (void*)0, (void**)&gop);
    if (status == EFI_SUCCESS) {
        params.framebuffer = (uint32*)gop->Mode->FrameBufferBase;
        params.width = gop->Mode->Info->HorizontalResolution;
        params.height = gop->Mode->Info->VerticalResolution;
        params.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    }

    /* 2. Load Files */
    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root;

    status = SystemTable->BootServices->HandleProtocol(ImageHandle, &li_g, (void**)&li);
    if (status == EFI_SUCCESS) {
        status = SystemTable->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs);
        if (status == EFI_SUCCESS) {
            status = fs->OpenVolume(fs, &root);
            if (status == EFI_SUCCESS) {
                /* Load Kernel at 16MB */
                status = load_file(SystemTable, root, L"kernel.bin", 0x1000000, (void*)0);
                if (status != EFI_SUCCESS) {
                    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error loading kernel.bin: ");
                    print_hex(SystemTable, status);
                    while(1);
                }

                /* Load Shell at 48MB */
                status = load_file(SystemTable, root, L"shell.bin", 0x3000000, &params.shell_size);
                if (status == EFI_SUCCESS) {
                    params.shell_base = (void*)0x3000000;
                }

                /* Allocate System Disk (Ramdisk) at 64MB */
                params.ramdisk_size = 16 * 1024 * 1024;
                EFI_PHYSICAL_ADDRESS disk_addr = 0x4000000;
                UINTN disk_pages = (params.ramdisk_size + 4095) / 4096;
                status = SystemTable->BootServices->AllocatePages(2, 2, disk_pages, &disk_addr);
                if (status == EFI_SUCCESS) {
                    params.ramdisk_base = (void*)disk_addr;
                    /* Zero the disk */
                    uint8* p = (uint8*)params.ramdisk_base;
                    for (uint64 i = 0; i < params.ramdisk_size; i++) p[i] = 0;
                }
            }
        }
    }

    /* 3. Allocate Heap at 32MB */
    UINTN heap_size = 8 * 1024 * 1024;
    EFI_PHYSICAL_ADDRESS heap_addr = 0x2000000;
    UINTN heap_pages = (heap_size + 4095) / 4096;
    status = SystemTable->BootServices->AllocatePages(2, 2, heap_pages, &heap_addr);
    if (status == EFI_SUCCESS) {
        params.heap_base = (void*)heap_addr;
        params.heap_size = (uint64)heap_size;
    } else {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error allocating heap: ");
        print_hex(SystemTable, status);
        while(1);
    }

    /* 4. The Beef Check */
    uint32 signature = *(volatile uint32*)0x1000000;
    if (signature != 0xDEADBEEF) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"WHERES_THE_BEEF! Checked: 0x1000000 | Found: ");
        print_hex(SystemTable, signature);
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L" | Expected: 0xDEADBEEF\r\n");
        while(1) { __asm__ volatile("hlt"); }
    }

    /* 5. Exit Boot Services */
    UINTN map_size = 0, map_key = 0, descriptor_size = 0;
    uint32 descriptor_version = 0;
    SystemTable->BootServices->GetMemoryMap(&map_size, (void*)0, &map_key, &descriptor_size, &descriptor_version);
    map_size += 2 * descriptor_size;
    void* map_buffer;
    if (SystemTable->BootServices->AllocatePool(2, map_size, &map_buffer) == EFI_SUCCESS) {
        if (SystemTable->BootServices->GetMemoryMap(&map_size, map_buffer, &map_key, &descriptor_size, &descriptor_version) == EFI_SUCCESS) {
            if (SystemTable->BootServices->ExitBootServices(ImageHandle, map_key) == EFI_SUCCESS) {
                /* Handover (Jump skipping the signature) */
                void (*kernel_start)(boot_params_t*) = (void (*)(boot_params_t*))0x1000004;
                kernel_start(&params);
            }
        }
    }

    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
