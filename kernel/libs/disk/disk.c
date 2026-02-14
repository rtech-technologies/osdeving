#include "disk.h"
#include "system.h"

#define MAX_RAMDISK_FILES 8

typedef struct {
    const char* name;
    void* data;
    UINTN size;
} ramdisk_file_t;

static ramdisk_file_t files[MAX_RAMDISK_FILES];
static UINTN file_count = 0;

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void disk_init() {
    // Already set up by registration
}

void disk_register_file(const char* name, void* data, UINTN size) {
    if (file_count < MAX_RAMDISK_FILES) {
        files[file_count].name = name;
        files[file_count].data = data;
        files[file_count].size = size;
        file_count++;
    }
}

INTN disk_read_file(const char* name, void* buffer, UINTN max_size) {
    // Handle leading slash
    if (name[0] == '/') name++;

    for (UINTN i = 0; i < file_count; i++) {
        const char* fname = files[i].name;
        if (fname[0] == '/') fname++;

        if (strcmp(fname, name) == 0) {
            UINTN to_copy = files[i].size;
            if (to_copy > max_size) to_copy = max_size;

            // Simple memcpy
            uint8* src = (uint8*)files[i].data;
            uint8* dst = (uint8*)buffer;
            for (UINTN j = 0; j < to_copy; j++) {
                dst[j] = src[j];
            }
            return (INTN)to_copy;
        }
    }
    return -1;
}
