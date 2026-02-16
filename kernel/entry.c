#include "../boot/efi_types.h"
#include "kernel.h"

static EFI_GUID gop_g = {0x70482061, 0x0512, 0x476A, {0xBC, 0xC3, 0x04, 0x54, 0x8F, 0x9E, 0x22, 0x07}};

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

    /* 2. Hand off to Kernel */
    kernel_main(&params);

    return EFI_SUCCESS;
}
