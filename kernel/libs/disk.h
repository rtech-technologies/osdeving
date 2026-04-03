#ifndef DISK_H
#define DISK_H

#include "../../include/types.h"

void disk_init();
int  read_sectors(uint64 lba, uint32 count, void* buffer);
int  write_sectors(uint64 lba, uint32 count, const void* buffer);

#endif
