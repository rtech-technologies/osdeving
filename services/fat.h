#ifndef FAT_H
#define FAT_H

#include "../include/types.h"

void fat_init();
/* FAT read-only interface for now */
INTN fat_read(const char* path, void* buffer, uint64 max_size);

#endif
