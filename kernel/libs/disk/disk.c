#include "disk.h"
#include "system.h"
#include "../memory/memory.h"

#define MAX_RAMDISK_FILES 8
#define RAMDISK_SIZE (8 * 1024 * 1024) // 8 MB
#define SECTOR_SIZE 512

typedef struct {
    const char* name;
    void* data;
    UINTN size;
} ramdisk_file_t;

static ramdisk_file_t files[MAX_RAMDISK_FILES];
static UINTN file_count = 0;
static uint8* disk_storage = NULL;

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void* memcpy(void* dest, const void* src, UINTN n) {
    uint8* d = dest;
    const uint8* s = src;
    for (UINTN i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void disk_init() {
    if (!disk_storage) {
        disk_storage = (uint8*)memory_alloc(RAMDISK_SIZE);
        if (disk_storage) {
            for (UINTN i = 0; i < RAMDISK_SIZE; i++) disk_storage[i] = 0;
        }
    }
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

            memcpy(buffer, files[i].data, to_copy);
            return (INTN)to_copy;
        }
    }
    return -1;
}

void read_sectors(uint64 lba, uint32 count, void *buffer) {
    if (!disk_storage) return;
    uint64 offset = lba * SECTOR_SIZE;
    uint64 size = (uint64)count * SECTOR_SIZE;
    if (offset + size > RAMDISK_SIZE) return;
    memcpy(buffer, disk_storage + offset, (UINTN)size);
}

void write_sectors(uint64 lba, uint32 count, const void *buffer) {
    if (!disk_storage) return;
    uint64 offset = lba * SECTOR_SIZE;
    uint64 size = (uint64)count * SECTOR_SIZE;
    if (offset + size > RAMDISK_SIZE) return;
    memcpy(disk_storage + offset, buffer, (UINTN)size);
}
