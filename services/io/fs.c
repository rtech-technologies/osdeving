#include "fs.h"
#include "../../kernel/kernel.h"

static EFI_GUID li_g = {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EFI_GUID fs_g = {0x0964E5B2, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

void fs_init() {
}

static void to_wstr(const char* src, CHAR16* dst, int max) {
    int i = 0;
    while (i < max - 1 && src[i]) {
        dst[i] = (CHAR16)src[i];
        i++;
    }
    dst[i] = 0;
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    if (!kboot_params.SystemTable) return -1;

    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root, *file;
    INTN read_bytes = -1;

    if (kboot_params.SystemTable->BootServices->HandleProtocol(kboot_params.ImageHandle, &li_g, (void**)&li) == EFI_SUCCESS) {
        if (kboot_params.SystemTable->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs) == EFI_SUCCESS) {
            if (fs->OpenVolume(fs, &root) == EFI_SUCCESS) {
                CHAR16 wpath[256];
                to_wstr(path, wpath, 256);

                if (root->Open(root, &file, wpath, 0x0000000000000001ULL, 0) == EFI_SUCCESS) {
                    UINTN size = (UINTN)max_size;
                    if (file->Read(file, &size, buffer) == EFI_SUCCESS) {
                        read_bytes = (INTN)size;
                    }
                    file->Close(file);
                }
                root->Close(root);
            }
        }
    }
    return read_bytes;
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    if (!kboot_params.SystemTable) return -1;

    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root, *file;
    INTN written_bytes = -1;

    if (kboot_params.SystemTable->BootServices->HandleProtocol(kboot_params.ImageHandle, &li_g, (void**)&li) == EFI_SUCCESS) {
        if (kboot_params.SystemTable->BootServices->HandleProtocol(li->DeviceHandle, &fs_g, (void**)&fs) == EFI_SUCCESS) {
            if (fs->OpenVolume(fs, &root) == EFI_SUCCESS) {
                CHAR16 wpath[256];
                to_wstr(path, wpath, 256);

                /* Open with Read/Write/Create (0x1 | 0x2 | 0x10) */
                if (root->Open(root, &file, wpath, 0x0000000000000001ULL | 0x0000000000000002ULL | 0x0000000000000010ULL, 0) == EFI_SUCCESS) {
                    UINTN bsize = (UINTN)size;
                    if (file->Write(file, &bsize, (void*)buffer) == EFI_SUCCESS) {
                        written_bytes = (INTN)bsize;
                    }
                    file->Close(file);
                }
                root->Close(root);
            }
        }
    }
    return written_bytes;
}
