#include <efi.h>
#include <efilib.h>
#include "disk.h"

static EFI_FILE_PROTOCOL* root_dir = NULL;
static EFI_HANDLE _image_handle;

void disk_init_with_handle(EFI_HANDLE image_handle) {
    _image_handle = image_handle;
}

void disk_init() {
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
    EFI_GUID fs_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;

    status = ST->BootServices->HandleProtocol(_image_handle, &loaded_image_protocol, (void**)&loaded_image);
    if (EFI_ERROR(status)) return;

    status = ST->BootServices->HandleProtocol(loaded_image->DeviceHandle, &fs_protocol, (void**)&fs);
    if (EFI_ERROR(status)) return;

    status = fs->OpenVolume(fs, &root_dir);
    if (EFI_ERROR(status)) root_dir = NULL;
}

EFI_FILE_PROTOCOL* disk_get_root() {
    return root_dir;
}
