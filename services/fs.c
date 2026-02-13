#include <efi.h>
#include <efilib.h>
#include "fs.h"
#include "disk.h"
#include "../include/system.h"

void fs_init() {
    // Registry initialization placeholder
}

INTN fs_read(const char* path, void* buffer, UINTN max_size) {
    EFI_FILE_PROTOCOL* root = disk_get_root();
    if (!root) return -1;

    CHAR16 wpath[256];
    UINTN i = 0;
    const char* p = path;
    if (p[0] == '/') p++;

    while (*p && i < 255) wpath[i++] = (CHAR16)(unsigned char)*p++;
    wpath[i] = 0;

    EFI_FILE_PROTOCOL* file;
    EFI_STATUS status = root->Open(root, &file, wpath, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) return -1;

    UINTN info_size = sizeof(EFI_FILE_INFO) + 256;
    EFI_FILE_INFO* info;
    status = ST->BootServices->AllocatePool(EfiLoaderData, info_size, (void**)&info);
    if (EFI_ERROR(status)) {
        file->Close(file);
        return -1;
    }

    status = file->GetInfo(file, &GenericFileInfo, &info_size, info);
    if (EFI_ERROR(status)) {
        ST->BootServices->FreePool(info);
        file->Close(file);
        return -1;
    }

    UINTN read_size = info->FileSize;
    if (read_size > max_size) {
        read_size = max_size;
    }

    ST->BootServices->FreePool(info);

    status = file->Read(file, &read_size, buffer);

    file->Close(file);
    return EFI_ERROR(status) ? -1 : (INTN)read_size;
}

INTN fs_write(const char* path, const void* buffer) {
    return -1;
}

INTN fread(const char* path, void* buffer, UINTN max_size) {
    return fs_read(path, buffer, max_size);
}

INTN fwrite(const char* path, const void* buffer) {
    return fs_write(path, buffer);
}
