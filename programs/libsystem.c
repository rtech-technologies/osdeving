#include "../include/sys"

static syscall_table_t* global_table = (void*)0;

void libsystem_init(syscall_table_t* table) {
    global_table = table;
}

void print(const char* str) {
    if (global_table) global_table->print(str);
}

void* alloc(size_t size) {
    if (global_table) return global_table->alloc(size);
    return (void*)0;
}

void free(void* ptr) {
    if (global_table) global_table->free(ptr);
}

INTN fread(const char* path, void* buffer, UINTN max_size) {
    if (global_table) return global_table->fread(path, buffer, max_size);
    return -1;
}

INTN fwrite(const char* path, const void* buffer) {
    if (global_table) return global_table->fwrite(path, buffer);
    return -1;
}

void wait_for_key() {
    if (global_table) global_table->wait_for_key();
}

char read_key() {
    if (global_table) return global_table->read_key();
    return 0;
}

void input(const char* prompt, char* buffer, size_t size) {
    if (global_table) global_table->input(prompt, buffer, size);
}

void lsdev() {
    if (global_table) global_table->lsdev();
}

void exit() {
    if (global_table) global_table->exit();
}
