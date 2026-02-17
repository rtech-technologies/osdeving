#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

/* Public API for programs - Category 12: Use custom types */
void print(const char* str);
void input(const char* prompt, char* buffer, uint64 size);
INTN fread(const char* path, void* buffer, uint64 max_size);
INTN fwrite(const char* path, const void* buffer, uint64 size);
void* alloc(uint64 size);
void free(void* ptr);
void exit();

/* Category: Syscall Table */
typedef struct {
    void (*print)(const char*);
    void (*input)(const char*, char*, uint64);
    INTN (*fread)(const char*, void*, uint64);
    INTN (*fwrite)(const char*, const void*, uint64);
    void* (*alloc)(uint64);
    void (*free)(void*);
    void (*exit)();
} syscall_table_t;

#endif
