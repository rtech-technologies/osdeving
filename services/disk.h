#ifndef DISK_H
#define DISK_H

#include "../kernel/kernel.h"

void disk_init();
EFI_FILE_PROTOCOL* disk_get_root();

#endif
