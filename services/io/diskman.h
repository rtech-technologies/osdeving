#ifndef DISKMAN_H
#define DISKMAN_H

#include "../../include/types.h"

void diskman_init();
void diskman_partition_efs();
void diskman_add_partition(uint64 start, uint32 count);
void diskman_format_rnafs(int idx);
void diskman_mount_rnafs(int idx);

#endif
