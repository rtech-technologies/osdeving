#include "../include/system.h"
#include "../kernel/kernel.h"

boot_params_t kboot_params;
static syscall_table_t* g_syscalls = NULL;

int program_main();

void print(const char* str) {
    if (g_syscalls && g_syscalls->print) g_syscalls->print(str);
}

void input(const char* prompt, char* buffer, uint64 size) {
    if (g_syscalls && g_syscalls->input) g_syscalls->input(prompt, buffer, size);
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    if (g_syscalls && g_syscalls->fread) return g_syscalls->fread(path, buffer, max_size);
    return -1;
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    if (g_syscalls && g_syscalls->fwrite) return g_syscalls->fwrite(path, buffer, size);
    return -1;
}

void* alloc(uint64 size) {
    if (g_syscalls && g_syscalls->alloc) return g_syscalls->alloc(size);
    return NULL;
}

void free(void* ptr) {
    if (g_syscalls && g_syscalls->free) g_syscalls->free(ptr);
}

void exit() {
    if (g_syscalls && g_syscalls->exit) g_syscalls->exit();
}

void format() {
    if (g_syscalls && g_syscalls->format) g_syscalls->format();
}

void lsfs() {
    if (g_syscalls && g_syscalls->lsfs) g_syscalls->lsfs();
}

void _start(boot_params_t* params, syscall_table_t* syscalls) {
    if (params) kboot_params = *params;
    if (syscalls) g_syscalls = syscalls;
    program_main();
}
