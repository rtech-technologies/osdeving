#ifndef LOADER_H
#define LOADER_H

#include "../../include/rsl.h"

void loader_init();
void loader_run_shell(rsl_syscall_table_t* syscalls);
void debug_memory_at_B0000();

#endif
