#include "fs.h"
#include "disk.h"

void fs_init() {
}

INTN fread(const char* path, void* buffer, UINTN max_size) {
    EFI_FILE_PROTOCOL* root = disk_get_root();
    if (!root) return -1;

    EFI_FILE_PROTOCOL* file;
    CHAR16 wpath[256];
    UINTN i = 0;
    while (path[i] && i < 255) {
        wpath[i] = (CHAR16)path[i];
        i++;
    }
    wpath[i] = 0;

    if (root->Open(root, &file, wpath, 1, 0) != EFI_SUCCESS) { // 1 is EFI_FILE_MODE_READ
        return -1;
    }

    UINTN size = max_size;
    if (file->Read(file, &size, buffer) != EFI_SUCCESS) {
        file->Close(file);
        return -1;
    }

    file->Close(file);
    return (INTN)size;
}

INTN fwrite(const char* path, const void* buffer, UINTN size) {
    return -1;
}
