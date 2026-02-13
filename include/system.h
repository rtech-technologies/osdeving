#ifndef SYSTEM_H
#define SYSTEM_H

#include "types.h"

// Public API for programs
void print(const char* str);
void* alloc(size_t size);
void free(void* ptr);
INTN fread(const char* path, void* buffer, UINTN max_size);
INTN fwrite(const char* path, const void* buffer);
void wait_for_key();
char read_key();
void exit();

// Syscall Table Structure for Kernel-to-Program interface
typedef struct {
    void (*print)(const char*);
    void* (*alloc)(size_t);
    void (*free)(void*);
    INTN (*fread)(const char*, void*, UINTN);
    INTN (*fwrite)(const char*, const void*);
    void (*wait_for_key)();
    char (*read_key)();
    void (*exit)();
} syscall_table_t;

#endif
