#ifndef FAT_H
#define FAT_H

#include "../../include/types.h"

void fat_init();
int fat_mount(uint64 partition_start_lba);

#endif
