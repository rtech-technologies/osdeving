#include "../include/sys"

extern int program_main();
extern void libsystem_init(syscall_table_t* table);

void _start(syscall_table_t* table) {
    libsystem_init(table);
    program_main();
}
