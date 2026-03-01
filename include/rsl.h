#ifndef RSL_H
#define RSL_H

#include "types.h"

/*
 * RTECH Standard Library (RSL)
 * The self-sustaining system language for OSx2 / RTECH dos.
 */

typedef struct {
    uint32* framebuffer;
    uint32  width;
    uint32  height;
    uint32  pixels_per_scanline;
    void*   ramdisk_base;
    uint64  ramdisk_size;
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
} rsl_syscall_table_t;

/* --- High-Level Friendly API --- */
void  print(const char* str);
char* readline(const char* prompt);
void* py_alloc(uint64 size);
void  py_free(void* ptr);
void  clear();
void  quit();

/* --- Core ARC System Calls --- */
void* alloc(uint64 size);
void  retain(void* ptr);
void  release(void* ptr);
INTN  read_file(const char* path, void* buffer, uint64 max_size);
INTN  write_file(const char* path, const void* buffer, uint64 size);

/* --- Utility Functions --- */
int    strcmp(const char* s1, const char* s2);
int    strncmp(const char* s1, const char* s2, uint64 n);
void   strcpy(char* dst, const char* src);
void   strncpy(char* dst, const char* src, uint64 n);
void   strcat(char* dst, const char* src);
uint64 strlen(const char* s);
char*  strchr(const char* s, int c);

void   memcpy(void* dst, const void* src, uint64 n);
void   memset(void* s, int c, uint64 n);

int    atoi(const char* s);
void   itoa(int n, char* s, int base);

#endif
