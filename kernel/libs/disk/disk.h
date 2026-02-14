#ifndef DISK_H
#define DISK_H

#include "types.h"

void disk_init();

// Platform interface to provide a pre-loaded file
void disk_register_file(const char* name, void* data, UINTN size);

// Generic read interface
INTN disk_read_file(const char* name, void* buffer, UINTN max_size);

#endif
