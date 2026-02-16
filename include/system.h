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
void input(const char* prompt, char* buffer, size_t size);
void lsdev();
void devman();
void exit();

// Diskman / FS Extended
void format();
void mount();
void lsfs();
INTN fwrite_sized(const char* path, const void* buffer, UINTN size);

// Syscall Table Structure
typedef struct {
    void (*print)(const char*);
    void* (*alloc)(size_t);
    void (*free)(void*);
    INTN (*fread)(const char*, void*, UINTN);
    INTN (*fwrite)(const char*, const void*);
    void (*wait_for_key)();
    char (*read_key)();
    void (*input)(const char*, char*, size_t);
    void (*lsdev)();
    void (*devman)();
    void (*exit)();

    void (*format)();
    void (*mount)();
    void (*lsfs)();
    INTN (*fwrite_sized)(const char*, const void*, UINTN);
} syscall_table_t;

#endif
