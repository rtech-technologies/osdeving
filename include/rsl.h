#ifndef RSL_H
#define RSL_H

#include "types.h"

/*
 * RTECH Standard Library (RSL)
 * The ultimate system language for OSx2 / RTECH dos.
 */

typedef struct {
    uint32* framebuffer;
    uint32  width;
    uint32  height;
    uint32  pixels_per_scanline;
    void*   ramdisk_base;
    uint64  ramdisk_size;
    void*   shell_base;
    uint64  shell_size;
    void*   heap_base;
    uint64  heap_size;
    void*   SystemTable;
    void*   ImageHandle;
} boot_params_t;

typedef struct {
    void (*print)(const char*);
    void (*input)(const char*, char*, uint64);
    INTN (*fread)(const char*, void*, uint64);
    INTN (*fwrite)(const char*, const void*, uint64);
    void* (*alloc)(uint64);
    void (*retain)(void*);
    void (*release)(void*);
    void (*exit)();
    void (*clear)();
    void (*set_color)(uint32 fg, uint32 bg);

    void (*format)(int);
    void (*mount)(int);
    void (*addpart)(uint64, uint32);
    void (*lsfs)();
} rsl_syscall_table_t;

/* --- High-Level "Noob-Friendly" API --- */
void  print(const char* str);
char* readline(const char* prompt);
void  clear();
void  quit();
void  color(uint32 fg, uint32 bg);

/* Disk & Partition Management */
void  format(int idx);
void  mount(int idx);
void  addpart(uint64 start, uint32 count);
void  lsfs();

/* --- Auto-RAM: Managed Memory API --- */
void* auto_ram(uint64 size);
void  release(void* ptr);
void  retain(void* ptr);

/* --- String Utilities --- */
char* str_create(const char* init);
int   strcmp(const char* s1, const char* s2);
int   strncmp(const char* s1, const char* s2, uint64 n);
void  strcpy(char* dst, const char* src);
void  strncpy(char* dst, const char* src, uint64 n);
uint64 strlen(const char* s);
char* strchr(const char* s, int c);

/* --- Memory & Files --- */
void* alloc(uint64 size);
void  memcpy(void* dst, const void* src, uint64 n);
void  memset(void* s, int c, uint64 n);
INTN  read_file(const char* path, void* buffer, uint64 max_size);
INTN  write_file(const char* path, const void* buffer, uint64 size);

/* --- Numeric Utilities --- */
int    atoi(const char* s);
void   itoa(int n, char* s, int base);

#endif
