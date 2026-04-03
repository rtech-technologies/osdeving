#include "../include/rsl.h"

/* RTECH Standard Library (RSL) Implementation */

static rsl_syscall_table_t* g_syscalls = (void*)0;
boot_params_t kboot_params;

int program_main();

void print(const char* str) {
    if (g_syscalls && g_syscalls->print) {
        g_syscalls->print(str);
    }
}

void* alloc(uint64 size) {
    if (g_syscalls && g_syscalls->alloc) {
        return g_syscalls->alloc(size);
    }
    return (void*)0;
}

void retain(void* ptr) {
    if (g_syscalls && g_syscalls->retain) {
        g_syscalls->retain(ptr);
    }
}

void release(void* ptr) {
    if (g_syscalls && g_syscalls->release) {
        g_syscalls->release(ptr);
    }
}

INTN read_file(const char* path, void* buffer, uint64 max_size) {
    if (g_syscalls && g_syscalls->fread) {
        return g_syscalls->fread(path, buffer, max_size);
    }
    return -1;
}

INTN write_file(const char* path, const void* buffer, uint64 size) {
    if (g_syscalls && g_syscalls->fwrite) {
        return g_syscalls->fwrite(path, buffer, size);
    }
    return -1;
}

void clear() {
    if (g_syscalls && g_syscalls->clear) {
        g_syscalls->clear();
    }
}

void quit() {
    if (g_syscalls && g_syscalls->exit) {
        g_syscalls->exit();
    }
}

void color(uint32 fg, uint32 bg) {
    if (g_syscalls && g_syscalls->set_color) {
        g_syscalls->set_color(fg, bg);
    }
}

void lsfs() {
}

void format(int idx) {
}

void mount(int idx) {
}

void addpart(uint64 start, uint32 count) {
    if (g_syscalls && g_syscalls->addpart) {
        g_syscalls->addpart(start, count);
    }
}

/* --- Auto-RAM: Simplified API --- */

void* auto_ram(uint64 size) {
    /* Everything in RSL is managed by ARC by default */
    return alloc(size);
}

char* str_create(const char* init) {
    uint64 len = strlen(init);
    char* s = (char*)auto_ram(len + 1);
    if (s) {
        strcpy(s, init);
    }
    return s;
}

char* readline(const char* prompt) {
    char* buf = (char*)auto_ram(128);
    if (!buf) return (void*)0;

    if (g_syscalls && g_syscalls->input) {
        g_syscalls->input(prompt, buf, 128);
    }
    return buf;
}

/* --- Utilities --- */

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, uint64 n) {
    for (uint64 i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
        if (s1[i] == 0) return 0;
    }
    return 0;
}

void strcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

void strncpy(char* dst, const char* src, uint64 n) {
    uint64 i;
    for (i = 0; i < n && src[i] != '\0'; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
}

uint64 strlen(const char* s) {
    uint64 n = 0;
    while (s[n]) n++;
    return n;
}

char* strchr(const char* s, int c) {
    while (s && *s != (char)c) {
        if (!*s++) return (void*)0;
    }
    return (char*)s;
}

void memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = (uint8*)dst;
    const uint8* s = (const uint8*)src;
    while (n--) *d++ = *s++;
}

void memset(void* s, int c, uint64 n) {
    uint8* p = (uint8*)s;
    while (n--) *p++ = (uint8)c;
}

int atoi(const char* s) {
    int res = 0;
    while (s && *s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

void itoa(int n, char* s, int base) {
    char* p = s;
    char* p1, *p2;
    unsigned int ud = n;
    if (base == 10 && n < 0) {
        *p++ = '-';
        s++;
        ud = -n;
    }
    do {
        int remainder = ud % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= base);
    *p = 0;
    p1 = s;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
}

void kernel_entry(boot_params_t* params, rsl_syscall_table_t* syscalls) {
    /* Serial Heartbeat 'C' (C Entry) */
    #define COM1 0x3F8
    __asm__ volatile ("outb %0, %1" : : "a"((unsigned char)'C'), "Nd"((unsigned short)COM1));

    if (params) kboot_params = *params;
    if (syscalls) g_syscalls = syscalls;
    program_main();
}
