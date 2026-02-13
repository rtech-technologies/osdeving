#ifndef FS_H
#define FS_H

#include "../include/types.h"

void fs_init();
INTN fs_read(const char* path, void* buffer, UINTN max_size);
INTN fs_write(const char* path, const void* buffer);

#endif
