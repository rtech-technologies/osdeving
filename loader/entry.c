#include <efi.h>
#include <efilib.h>
#include "../include/config.h"

/* Manual structure for boot_params to avoid conflicting types.h */
typedef struct {
    UINT32* framebuffer;
    UINT32  width;
    UINT32  height;
    UINT32  pixels_per_scanline;
    void*   ramdisk_base;
    UINT64  ramdisk_size;
    void*   shell_base;
    UINT64  shell_size;
    void*   heap_base;
    UINT64  heap_size;
    void*   SystemTable;
    void*   ImageHandle;
} loader_params_t;

static EFI_GUID li_g = {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID fs_g = {0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID gop_g = {0x70482061, 0x0512, 0x476A, {0xBC, 0xC3, 0x04, 0x54, 0x8F, 0x9E, 0x22, 0x07}};
static EFI_GUID info_g = {0x0964E5B2, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

static EFI_STATUS load_file(EFI_SYSTEM_TABLE *ST, EFI_FILE_PROTOCOL *root, CHAR16 *name, EFI_PHYSICAL_ADDRESS addr, UINT64 *out_size) {
    EFI_FILE_PROTOCOL *file;
    EFI_STATUS status = root->Open(root, &file, name, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS) return status;

    EFI_FILE_INFO *info = LibFileInfo(file);
    if (!info) return EFI_LOAD_ERROR;

    UINT64 file_size = info->FileSize;
    UINTN pages = (file_size + 4095) / 4096;
    EFI_PHYSICAL_ADDRESS load_addr = addr;

    status = ST->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &load_addr);
    if (status != EFI_SUCCESS) return status;

    UINTN read_size = (UINTN)file_size;
    status = file->Read(file, &read_size, (void*)load_addr);
    if (status == EFI_SUCCESS && out_size) *out_size = (UINT64)read_size;

    FreePool(info);
    file->Close(file);
    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    loader_params_t params = {0};
    EFI_STATUS status;

    Print(L"OS*2 Loader: Locating Opaque Sheep...\n");

    /* 1. Get Graphics Info */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = SystemTable->BootServices->LocateProtocol(&gop_g, (void*)0, (void**)&gop);
    if (status == EFI_SUCCESS) {
        params.framebuffer = (UINT32*)gop->Mode->FrameBufferBase;
        params.width = gop->Mode->Info->HorizontalResolution;
        params.height = gop->Mode->Info->VerticalResolution;
        params.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    }

    /* 2. Load Files */
    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root;

    status = SystemTable->BootServices->HandleProtocol(ImageHandle, &li_g, (void**)&li);
    if (status != EFI_SUCCESS) {
        Print(L"FATAL: LoadedImage Protocol Failed! %r\n", status);
        while(1);
    }

    status = SystemTable->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs);
    if (status != EFI_SUCCESS) {
        Print(L"FATAL: FileSystem Protocol Failed! %r\n", status);
        while(1);
    }

    status = fs->OpenVolume(fs, &root);
    if (status != EFI_SUCCESS) {
        Print(L"FATAL: Could not open root volume! %r\n", status);
        while(1);
    }

    /* Load Kernel at configured base */
    status = load_file(SystemTable, root, L"os2.bin", CONFIG_KERNEL_BASE, (void*)0);
    if (status != EFI_SUCCESS) {
        Print(L"FATAL: Error loading os2.bin: %r\n", status);
        while(1);
    }

    /* Load Shell at 48MB */
    status = load_file(SystemTable, root, L"shell.bin", 0x3000000, &params.shell_size);
    if (status == EFI_SUCCESS) {
        params.shell_base = (void*)0x3000000;
    } else {
        Print(L"Warning: shell.bin not found. %r\n", status);
    }

    /* Allocate System Disk (Ramdisk) at 64MB */
    params.ramdisk_size = 16 * 1024 * 1024;
    EFI_PHYSICAL_ADDRESS disk_addr = 0x4000000;
    UINTN disk_pages = (params.ramdisk_size + 4095) / 4096;
    status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, disk_pages, &disk_addr);
    if (status == EFI_SUCCESS) {
        params.ramdisk_base = (void*)disk_addr;
        /* Zero the disk */
        UINT8* p = (UINT8*)params.ramdisk_base;
        for (UINT64 i = 0; i < params.ramdisk_size; i++) p[i] = 0;
    } else {
        Print(L"FATAL: Ramdisk allocation failed! %r\n", status);
        while(1);
    }

    /* 3. Allocate Heap */
    UINTN heap_size = (UINTN)CONFIG_HEAP_SIZE_MB * 1024 * 1024;
    EFI_PHYSICAL_ADDRESS heap_addr = CONFIG_HEAP_BASE;
    UINTN heap_pages = (heap_size + 4095) / 4096;
    status = SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, heap_pages, &heap_addr);
    if (status == EFI_SUCCESS) {
        params.heap_base = (void*)heap_addr;
        params.heap_size = (UINT64)heap_size;
    } else {
        Print(L"Error allocating heap: %r\n", status);
        while(1);
    }

    /* 4. The Beef Check */
    UINT32 signature = *(volatile UINT32*)CONFIG_KERNEL_BASE;
    if (signature != 0xDEADBEEF) {
        Print(L"WHERES_THE_BEEF! Checked: 0x%lx | Found: 0x%x | Expected: 0xDEADBEEF\n", (UINT64)CONFIG_KERNEL_BASE, signature);
        while(1) { __asm__ volatile("hlt"); }
    }

    Print(L"Sheep Loaded. Breaking the walls...\n");

    /* 5. Exit Boot Services */
    UINTN map_size = 0, map_key = 0, descriptor_size = 0;
    UINT32 descriptor_version = 0;
    SystemTable->BootServices->GetMemoryMap(&map_size, (void*)0, &map_key, &descriptor_size, &descriptor_version);
    map_size += 2 * descriptor_size;
    void* map_buffer;
    if (SystemTable->BootServices->AllocatePool(2, map_size, &map_buffer) == EFI_SUCCESS) {
        if (SystemTable->BootServices->GetMemoryMap(&map_size, map_buffer, &map_key, &descriptor_size, &descriptor_version) == EFI_SUCCESS) {
            if (SystemTable->BootServices->ExitBootServices(ImageHandle, map_key) == EFI_SUCCESS) {
                /* Handover (Jump skipping the signature) */
                void (*kernel_start_f)(loader_params_t*) = (void (*)(loader_params_t*))(CONFIG_KERNEL_BASE + 4);
                kernel_start_f(&params);
            }
        }
    }

    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
