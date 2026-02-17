#include "fs.h"
#include "rnafs.h"
#include "../../kernel/kernel.h"

void fs_init() {
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(uint8*)s1 - *(uint8*)s2;
}

static void memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = dst;
    const uint8* s = src;
    for (uint64 i = 0; i < n; i++) d[i] = s[i];
}

INTN fread(const char* path, void* buffer, uint64 max_size) {
    const char* name = path;
    if (name[0] == '/') name++;

    /* v0 fallback: check if it's the shell preloaded by bootloader */
    if (strcmp(name, "shell.bin") == 0 && kboot_params.ramdisk_base) {
        uint64 to_copy = (kboot_params.ramdisk_size < max_size) ? kboot_params.ramdisk_size : max_size;
        memcpy(buffer, kboot_params.ramdisk_base, to_copy);
        return (INTN)to_copy;
    }

    return (INTN)rnafs_read_file(name, buffer, (uint32)max_size);
}

INTN fwrite(const char* path, const void* buffer, uint64 size) {
    const char* name = path;
    if (name[0] == '/') name++;
    return (INTN)rnafs_write_file(name, buffer, (uint32)size);
}
