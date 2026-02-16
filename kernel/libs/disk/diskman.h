#ifndef DISKMAN_H
#define DISKMAN_H

#include "types.h"

void diskman_init();
void diskman_format();
void diskman_mount();
void diskman_ls();

void diskman_add_partition(uint64 start_lba, uint32 sector_count);
void diskman_format_fat(int partition_index);

#endif
