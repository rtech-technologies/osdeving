#include "../boot/efi_types.h"
#include "kernel.h"

/* EFI Memory pool types */
#define EFI_BOOT_SERVICES_DATA 6
#define EFI_LOADER_DATA 2

/* EFI File open modes */
#define EFI_FILE_MODE_READ 0x0001

static EFI_GUID gop_g = {0x70482061, 0x0512, 0x476A, {0xBC, 0xC3, 0x04, 0x54, 0x8F, 0x9E, 0x22, 0x07}};
static EFI_GUID li_g = {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID fs_g = {0x0964E5B2, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID info_g = {0x0964e5b2, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    boot_params_t params = {0};

    /* 1. Get Graphics Info */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    if (SystemTable->BootServices->LocateProtocol(&gop_g, NULL, (void**)&gop) == EFI_SUCCESS && gop && gop->Mode) {
        params.framebuffer = (uint32*)gop->Mode->FrameBufferBase;
        params.width = gop->Mode->Info->HorizontalResolution;
        params.height = gop->Mode->Info->VerticalResolution;
        params.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    } else {
        /* Fallback framebuffer - assumes linear framebuffer at 0xA0000 (legacy VGA) */
        params.framebuffer = (uint32*)0xA0000;
        params.width = 80;
        params.height = 25;
        params.pixels_per_scanline = 80;
    }

    /* 2. Load shell.bin into Memory */
    EFI_LOADED_IMAGE_PROTOCOL *li = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
    EFI_FILE_PROTOCOL *root = NULL, *file = NULL;

    if (SystemTable->BootServices->HandleProtocol(ImageHandle, &li_g, (void**)&li) == EFI_SUCCESS && li) {
        if (SystemTable->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs) == EFI_SUCCESS && fs) {
            if (fs->OpenVolume(fs, &root) == EFI_SUCCESS && root) {
                if (root->Open(root, &file, L"shell.bin", EFI_FILE_MODE_READ, 0) == EFI_SUCCESS && file) {
                    /* Allocate larger buffer for file info structure */
                    uint8 info_buffer[512];
                    UINTN info_size = sizeof(info_buffer);
                    uint64 file_size = 65536;

                    if (file->GetInfo(file, &info_g, &info_size, info_buffer) == EFI_SUCCESS) {
                        file_size = ((EFI_FILE_INFO*)info_buffer)->FileSize;
                    }

                    UINTN size = (UINTN)file_size;
                    void* buffer = NULL;
                    if (SystemTable->BootServices->AllocatePool(EFI_BOOT_SERVICES_DATA, size, &buffer) == EFI_SUCCESS && buffer) {
                        if (file->Read(file, &size, buffer) == EFI_SUCCESS) {
                            params.ramdisk_base = buffer;
                            params.ramdisk_size = (uint64)size;
                        } else {
                            /* Read failed, free allocated buffer */
                            SystemTable->BootServices->FreePool(buffer);
                        }
                    }
                    file->Close(file);
                }
                root->Close(root);
            }
        }
    }

    /* 3. Allocate Heap */
    UINTN heap_size = 4 * 1024 * 1024;
    void* heap_base = NULL;
    if (SystemTable->BootServices->AllocatePool(EFI_BOOT_SERVICES_DATA, heap_size, &heap_base) == EFI_SUCCESS && heap_base) {
        params.heap_base = heap_base;
        params.heap_size = heap_size;
    }

    params.SystemTable = SystemTable;
    params.ImageHandle = ImageHandle;

    /* 4. Hand off to Kernel */
    kernel_main(&params);

    /* 5. Kernel should not return, but if it does, halt indefinitely */
    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
