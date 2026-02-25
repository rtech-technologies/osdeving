#ifndef RRDFS_H
#define RRDFS_H

#include "../include/types.h"

void rrdfs_init();
INTN rrdfs_read(const char* path, void* buffer, uint64 max_size);
INTN rrdfs_write(const char* path, const void* buffer, uint64 size);

#endif
