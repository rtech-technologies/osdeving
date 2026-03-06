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

    /* 2. Load shell.bin into Specific Address 0x200000 */
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

                    /* Force Load at 0x200000 */
                    EFI_PHYSICAL_ADDRESS kernel_addr = 0x200000;
                    UINTN pages = (file_size + 4095) / 4096;

                    /* AllocateAddress (type 2) forces UEFI to use the specified addr */
                    if (SystemTable->BootServices->AllocatePages(2, 1, pages, &kernel_addr) == EFI_SUCCESS) {
                        UINTN read_size = (UINTN)file_size;
                        if (file->Read(file, &read_size, (void*)kernel_addr) == EFI_SUCCESS) {
                            params.ramdisk_base = (void*)kernel_addr;
                            params.ramdisk_size = (uint64)read_size;
                        }
                    } else {
                        /* Fallback or Error */
                    }
                    file->Close(file);
                }
                root->Close(root);
            }
        }
    }

    /* 3. Pre-allocate Heap (Safe spot above kernel) */
    UINTN heap_size = 4 * 1024 * 1024;
    EFI_PHYSICAL_ADDRESS heap_addr = 0x1000000; /* 16MB mark */
    UINTN heap_pages = (heap_size + 4095) / 4096;

    if (SystemTable->BootServices->AllocatePages(2, 2, heap_pages, &heap_addr) == EFI_SUCCESS) {
        params.heap_base = (void*)heap_addr;
        params.heap_size = heap_size;
    }

    /* 4. Exit Boot Services Ritual */
    UINTN map_size = 0;
    UINTN map_key = 0;
    UINTN descriptor_size = 0;
    uint32 descriptor_version = 0;

    SystemTable->BootServices->GetMemoryMap(&map_size, NULL, &map_key, &descriptor_size, &descriptor_version);
    map_size += 2 * descriptor_size;

    void* map_buffer;
    if (SystemTable->BootServices->AllocatePool(2, map_size, &map_buffer) == EFI_SUCCESS) {
        if (SystemTable->BootServices->GetMemoryMap(&map_size, map_buffer, &map_key, &descriptor_size, &descriptor_version) == EFI_SUCCESS) {
            if (SystemTable->BootServices->ExitBootServices(ImageHandle, map_key) == EFI_SUCCESS) {
                /* Freestanding handoff */
                kernel_main(&params);
            }
        }
    }

    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
