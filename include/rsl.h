#ifndef RSL_H
#define RSL_H

#include "types.h"

/* RTECH Standard Library (RSL) for OSx2 / RTECH dos */

/* Boot parameters and syscall definitions */
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
    void (*free)(void*);
    void (*exit)();
} rsl_syscall_table_t;

/* Public API (RSL Functions) */
void rsl_print(const char* str);
void rsl_input(const char* prompt, char* buffer, uint64 size);
INTN rsl_fread(const char* path, void* buffer, uint64 max_size);
INTN rsl_fwrite(const char* path, const void* buffer, uint64 size);
void* rsl_alloc(uint64 size);
void rsl_free(void* ptr);
void rsl_exit();

/* RSL Utility Functions */
int  rsl_strcmp(const char* s1, const char* s2);
int  rsl_strncmp(const char* s1, const char* s2, uint64 n);
void rsl_memcpy(void* dst, const void* src, uint64 n);
void rsl_memset(void* s, int c, uint64 n);
uint64 rsl_strlen(const char* s);

#endif
