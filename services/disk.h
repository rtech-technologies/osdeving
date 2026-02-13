#ifndef DISK_H
#define DISK_H

#include <efi.h>

void disk_init_with_handle(EFI_HANDLE image_handle);
void disk_init();
EFI_FILE_PROTOCOL* disk_get_root();

#endif
