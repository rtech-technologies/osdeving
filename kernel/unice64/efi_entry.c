#include <efi.h>
#include <efilib.h>
#include "../../include/config.h"
#include "../../include/types.h"
#include "../../include/rsl.h"
#include "../libs/kutils.h"
#include "kernel.h"

static EFI_GUID li_g = LOADED_IMAGE_PROTOCOL;
static EFI_GUID fs_g = SIMPLE_FILE_SYSTEM_PROTOCOL;
static EFI_GUID gop_g = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

static EFI_STATUS load_file(EFI_SYSTEM_TABLE *ST, EFI_FILE_PROTOCOL *root, CHAR16 *name, EFI_PHYSICAL_ADDRESS addr, EFI_ALLOCATE_TYPE type, EFI_MEMORY_TYPE mem_type, UINT64 *out_size, void **out_ptr) {
    if (!root) return EFI_INVALID_PARAMETER;
    EFI_FILE_PROTOCOL *file;
    EFI_STATUS status = root->Open(root, &file, name, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS) return status;

    EFI_FILE_INFO *info = LibFileInfo(file);
    if (!info) {
        file->Close(file);
        return EFI_LOAD_ERROR;
    }

    UINT64 file_size = info->FileSize;
    UINTN pages = (file_size + 4095) / 4096;
    EFI_PHYSICAL_ADDRESS load_addr = addr;

    status = ST->BootServices->AllocatePages(type, mem_type, pages, &load_addr);
    if (status != EFI_SUCCESS) {
        FreePool(info);
        file->Close(file);
        return status;
    }

    UINTN read_size = (UINTN)file_size;
    status = file->Read(file, &read_size, (void*)load_addr);
    if (status == EFI_SUCCESS) {
        if (out_size) *out_size = (UINT64)read_size;
        if (out_ptr) *out_ptr = (void*)load_addr;
    }

    FreePool(info);
    file->Close(file);
    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    serial_init();
    boot_params_t params = {0};
    EFI_STATUS status;

    Print(L"OSx2 Native EFI Kernel Booting...\n");
    serial_print("OSx2: UEFI Entry Point reached.\n");

    /* 1. Get Graphics Info */
    Print(L"EFI: Initializing Graphics (GOP)...\n");
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = SystemTable->BootServices->LocateProtocol(&gop_g, (void*)0, (void**)&gop);
    if (status == EFI_SUCCESS) {
        params.framebuffer = (UINT32*)gop->Mode->FrameBufferBase;
        params.width = gop->Mode->Info->HorizontalResolution;
        params.height = gop->Mode->Info->VerticalResolution;
        params.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
        Print(L"EFI: Graphics initialized (%ux%u).\n", params.width, params.height);
    }

    /* 2. Load Shell */
    Print(L"EFI: Retrieving Protocols...\n");
    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root;

    status = SystemTable->BootServices->OpenProtocol(ImageHandle, &li_g, (void**)&li, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (status != EFI_SUCCESS) {
        Print(L"FATAL: LoadedImage Protocol Failed! %r\n", status);
        while(1);
    }

    fs = NULL;
    status = SystemTable->BootServices->OpenProtocol(li->DeviceHandle, &fs_g, (void**)&fs, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (status != EFI_SUCCESS || fs == NULL) {
        Print(L"Opaque Error: FileSystem is NULL! (Status: %r)\n", status);
        Print(L"This usually means the boot partition is not correctly recognized.\n");
        while(1);
    }

    Print(L"EFI: Opening Boot Volume...\n");
    status = fs->OpenVolume(fs, &root);
    if (status != EFI_SUCCESS || root == NULL) {
        Print(L"Opaque Error: Root Volume is NULL! (Status: %r)\n", status);
        while(1);
    }

    /* Single-Partition Strategy: os2.bin is on the same volume as the loader */
    EFI_FILE_PROTOCOL* os_root = root;

    /* Load Kernel (Sheep) at 48MB (or wherever specified) */
    /* Using EfiLoaderCode for execution compatibility */
    Print(L"EFI: Loading Kernel (os2.bin)...\n");
    UINT64 shell_size = 0;
    void* shell_base = NULL;

    /* Overlap Check: Ensure Stage 2 doesn't hit the framebuffer */
    EFI_PHYSICAL_ADDRESS fb_base = (EFI_PHYSICAL_ADDRESS)params.framebuffer;
    EFI_PHYSICAL_ADDRESS fb_end = fb_base + (params.height * params.pixels_per_scanline * 4);
    if (0x3000000 < fb_end && (0x3000000 + 16*1024*1024) > fb_base) {
        Print(L"Opaque Error: Stage 2 load address overlaps with Framebuffer!\n");
        while(1);
    }

    status = load_file(SystemTable, os_root, L"os2.bin", 0x3000000, AllocateAddress, EfiLoaderCode, &shell_size, &shell_base);
    if (status == EFI_SUCCESS) {
        params.shell_size = (uint64)shell_size;
        params.shell_base = shell_base;
        Print(L"EFI: Kernel loaded at %p (%lu bytes).\n", params.shell_base, params.shell_size);
    } else {
        Print(L"Warning: os2.bin not found on OS partition. %r\n", status);
    }

    /* 3. Prepare System Disk (Ramdisk) */
    /* Attempt to load ramdisk.img from boot volume, otherwise create blank */
    Print(L"EFI: Preparing RAM Disk...\n");
    UINT64 ramdisk_size = 512 * 1024 * 1024; /* Support up to 512MB ramdisk */
    void* ramdisk_base = NULL;
    status = load_file(SystemTable, os_root, L"ramdisk.img", 0, AllocateAnyPages, EfiLoaderData, &ramdisk_size, &ramdisk_base);
    params.ramdisk_size = (uint64)ramdisk_size;
    if (status == EFI_SUCCESS) {
        params.ramdisk_base = ramdisk_base;
        Print(L"EFI: RAM Disk loaded at %p (%lu bytes).\n", params.ramdisk_base, params.ramdisk_size);
    } else {
        /* Create fallback zeroed ramdisk */
        EFI_PHYSICAL_ADDRESS disk_addr = 0;
        UINTN disk_pages = (params.ramdisk_size + 4095) / 4096;
        status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, disk_pages, &disk_addr);
        if (status == EFI_SUCCESS) {
            params.ramdisk_base = (void*)disk_addr;
            /* Zero the disk */
            UINT8* p = (UINT8*)params.ramdisk_base;
            for (UINT64 i = 0; i < params.ramdisk_size; i++) p[i] = 0;
        }
    }

    /* 4. Allocate Heap */
    Print(L"EFI: Allocating Kernel Heap...\n");
    UINTN heap_size = (UINTN)CONFIG_HEAP_SIZE_MB * 1024 * 1024;
    EFI_PHYSICAL_ADDRESS heap_addr = 0;
    UINTN heap_pages = (heap_size + 4095) / 4096;
    status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, heap_pages, &heap_addr);
    if (status == EFI_SUCCESS) {
        params.heap_base = (void*)heap_addr;
        params.heap_size = (uint64)heap_size;
        /* Zero the heap */
        UINT8* p = (UINT8*)params.heap_base;
        for (UINT64 i = 0; i < (UINT64)heap_size; i++) p[i] = 0;
        Print(L"EFI: Heap allocated at %p (%lu MB).\n", params.heap_base, (uint64)CONFIG_HEAP_SIZE_MB);
    }

    params.SystemTable = SystemTable;
    params.ImageHandle = ImageHandle;

    Print(L"EFI: Handover complete. Exiting Boot Services...\n");

    /* 5. Exit Boot Services with Retry Loop */
    UINTN map_size = 0, map_key = 0, descriptor_size = 0;
    UINT32 descriptor_version = 0;
    void* map_buffer = NULL;
    int retry = 5;

    while (retry--) {
        status = SystemTable->BootServices->GetMemoryMap(&map_size, (void*)0, &map_key, &descriptor_size, &descriptor_version);
        map_size += 4 * descriptor_size; /* Buffer for potential growth */

        status = SystemTable->BootServices->AllocatePool(EfiLoaderData, map_size, &map_buffer);
        if (status != EFI_SUCCESS) break;

        status = SystemTable->BootServices->GetMemoryMap(&map_size, map_buffer, &map_key, &descriptor_size, &descriptor_version);
        if (status == EFI_SUCCESS) {
            status = SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);
            if (status == EFI_SUCCESS) {
                /* SUCCESS: No more Boot Services calls allowed! */
                kernel_main(&params);
            }
        }
        /* Only free pool if ExitBootServices failed */
        SystemTable->BootServices->FreePool(map_buffer);
        map_buffer = NULL;
    }

    Print(L"FATAL: ExitBootServices failed after retries!\n");
    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
