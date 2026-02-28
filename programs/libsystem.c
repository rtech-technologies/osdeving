#include "../include/rsl.h"

/* RTECH Standard Library (RSL) Implementation for OSx2 / RTECH dos */

static rsl_syscall_table_t* g_syscalls = NULL;
boot_params_t kboot_params;

int program_main();

void rsl_print(const char* str) {
    if (g_syscalls && g_syscalls->print) g_syscalls->print(str);
}

void rsl_input(const char* prompt, char* buffer, uint64 size) {
    if (g_syscalls && g_syscalls->input) g_syscalls->input(prompt, buffer, size);
}

INTN rsl_fread(const char* path, void* buffer, uint64 max_size) {
    if (g_syscalls && g_syscalls->fread) return g_syscalls->fread(path, buffer, max_size);
    return -1;
}

INTN rsl_fwrite(const char* path, const void* buffer, uint64 size) {
    if (g_syscalls && g_syscalls->fwrite) return g_syscalls->fwrite(path, buffer, size);
    return -1;
}

void* rsl_alloc(uint64 size) {
    if (g_syscalls && g_syscalls->alloc) return g_syscalls->alloc(size);
    return NULL;
}

void rsl_free(void* ptr) {
    if (g_syscalls && g_syscalls->free) g_syscalls->free(ptr);
}

void rsl_exit() {
    if (g_syscalls && g_syscalls->exit) g_syscalls->exit();
}

/* RSL Utility Functions */
int rsl_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int rsl_strncmp(const char* s1, const char* s2, uint64 n) {
    for (uint64 i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
        if (s1[i] == 0) return 0;
    }
    return 0;
}

void rsl_memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = (uint8*)dst;
    const uint8* s = (const uint8*)src;
    for (uint64 i = 0; i < n; i++) d[i] = s[i];
}

void rsl_memset(void* s, int c, uint64 n) {
    uint8* p = (uint8*)s;
    for (uint64 i = 0; i < n; i++) p[i] = (uint8)c;
}

uint64 rsl_strlen(const char* s) {
    uint64 n = 0;
    while (s[n]) n++;
    return n;
}

/* Program Entry Stub */
void _start(boot_params_t* params, rsl_syscall_table_t* syscalls) {
    if (params) kboot_params = *params;
    if (syscalls) g_syscalls = syscalls;
    program_main();
}
