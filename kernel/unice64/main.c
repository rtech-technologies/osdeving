#include <efi.h>
#include <efilib.h>
#include "sys"

#define KERNEL_HEAP_SIZE (4 * 1024 * 1024)

static void* load_asset(EFI_HANDLE img, const CHAR16* path, UINTN* sz) {
    EFI_LOADED_IMAGE_PROTOCOL* li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root, *f;
    EFI_GUID li_g = LOADED_IMAGE_PROTOCOL, fs_g = SIMPLE_FILE_SYSTEM_PROTOCOL;

    if (EFI_ERROR(uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, img, &li_g, (void**)&li))) return NULL;
    if (EFI_ERROR(uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, li->DeviceHandle, &fs_g, (void**)&fs))) return NULL;
    if (uefi_call_wrapper(fs->OpenVolume, 2, fs, &root) != EFI_SUCCESS) return NULL;
    if (uefi_call_wrapper(root->Open, 5, root, &f, (CHAR16*)path, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS) return NULL;

    UINTN isz = sizeof(EFI_FILE_INFO) + 256;
    EFI_FILE_INFO* info;
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, isz, (void**)&info);
    uefi_call_wrapper(f->GetInfo, 4, f, &GenericFileInfo, &isz, info);

    *sz = info->FileSize;
    void* b;
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, *sz, &b);
    uefi_call_wrapper(f->Read, 3, f, sz, b);

    uefi_call_wrapper(ST->BootServices->FreePool, 1, info);
    uefi_call_wrapper(f->Close, 1, f);
    return b;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    void* heap = NULL;
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, KERNEL_HEAP_SIZE, &heap);
    if (heap) memory_set_heap(heap, KERNEL_HEAP_SIZE);

    boot_params_t p = {0};
    EFI_GUID g_g = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    if (!EFI_ERROR(uefi_call_wrapper(ST->BootServices->LocateProtocol, 3, &g_g, NULL, (void**)&gop))) {
        p.framebuffer = (uint32*)gop->Mode->FrameBufferBase;
        p.width = gop->Mode->Info->HorizontalResolution;
        p.height = gop->Mode->Info->VerticalResolution;
        p.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;
    }

    UINTN sh_sz = 0;
    void* sh_data = load_asset(ImageHandle, L"shell.bin", &sh_sz);
    if (sh_data) disk_register_file("shell.bin", sh_data, sh_sz);

    stup(&p);
    main();

    while(1) { __asm__ volatile("hlt"); }
    return EFI_SUCCESS;
}
