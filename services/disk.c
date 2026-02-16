#include "disk.h"

static EFI_FILE_PROTOCOL* root_dir = NULL;

static EFI_GUID li_g = {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID fs_g = {0x0964E5B2, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

void disk_init() {
    if (!ST_PTR || !ST_PTR->BootServices) return;

    EFI_LOADED_IMAGE_PROTOCOL* li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

    if (ST_PTR->BootServices->HandleProtocol(ImageHandle_PTR, &li_g, (void**)&li) != EFI_SUCCESS) return;
    if (ST_PTR->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs) != EFI_SUCCESS) return;
    if (fs->OpenVolume(fs, &root_dir) != EFI_SUCCESS) return;
}

EFI_FILE_PROTOCOL* disk_get_root() {
    return root_dir;
}
