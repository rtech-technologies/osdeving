#ifndef FS_H
#define FS_H

#include "../../include/types.h"

void fs_init();
INTN fread(const char* path, void* buffer, uint64 max_size);
INTN fwrite(const char* path, const void* buffer, uint64 size);

#endif
