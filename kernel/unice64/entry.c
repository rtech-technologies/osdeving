#include "../../boot/efi_types.h"
#include "kernel.h"

static EFI_GUID gop_g = {0x70482061, 0x0512, 0x476A, {0xBC, 0xC3, 0x04, 0x54, 0x8F, 0x9E, 0x22, 0x07}};
static EFI_GUID li_g = {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID fs_g = {0x0964E5B2, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID info_g = {0x0964e5b2, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    boot_params_t params = {0};

    /* 1. Get Graphics Info */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    if (SystemTable->BootServices->LocateProtocol(&gop_g, NULL, (void**)&gop) == EFI_SUCCESS) {
        params.framebuffer = (uint32*)gop->Mode->FrameBufferBase;
        params.width = gop->Mode->Info->HorizontalResolution;
        params.height = gop->Mode->Info->VerticalResolution;
        params.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    }

    /* 2. Load shell.bin into Memory */
    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root, *file;

    if (SystemTable->BootServices->HandleProtocol(ImageHandle, &li_g, (void**)&li) == EFI_SUCCESS) {
        if (SystemTable->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs) == EFI_SUCCESS) {
            if (fs->OpenVolume(fs, &root) == EFI_SUCCESS) {
                if (root->Open(root, &file, L"shell.bin", 1, 0) == EFI_SUCCESS) {
                    uint8 info_buffer[256];
                    UINTN info_size = sizeof(info_buffer);
                    uint64 file_size = 65536;

                    if (file->GetInfo(file, &info_g, &info_size, info_buffer) == EFI_SUCCESS) {
                        file_size = ((EFI_FILE_INFO*)info_buffer)->FileSize;
                    }

                    UINTN size = (UINTN)file_size;
                    void* buffer;
                    if (SystemTable->BootServices->AllocatePool(1, size, &buffer) == EFI_SUCCESS) {
                        if (file->Read(file, &size, buffer) == EFI_SUCCESS) {
                            params.ramdisk_base = buffer;
                            params.ramdisk_size = (uint64)size;
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
    void* heap_base;
    if (SystemTable->BootServices->AllocatePool(2, heap_size, &heap_base) == EFI_SUCCESS) {
        params.heap_base = heap_base;
        params.heap_size = heap_size;
    }

    params.SystemTable = SystemTable;
    params.ImageHandle = ImageHandle;

    /* 4. Hand off to Kernel */
    kernel_main(&params);

    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
