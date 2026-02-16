#include "rnafs.h"
#include "console.h"
#include "memory.h"
#include "../disk/disk.h"

// Standard RNAFS v1 Implementation

static uint64 rnafs_partition_start_lba;
static uint32 rnafs_total_blocks;

static uint8* rnafs_bitmap = NULL;
static DirEntry* rnafs_dir_table = NULL;
static uint8 rnafs_block_buffer[FS_BLOCK_SIZE];

static uint32 bitmap_block_count;
static uint32 dir_block_count;
static uint32 data_start_block;

#define DIR_ENTRY_COUNT (dir_block_count * (FS_BLOCK_SIZE / sizeof(DirEntry)))

int rnafs_ready = 0;

static void* memset(void* s, int c, UINTN n) {
    uint8* p = s;
    while (n--) *p++ = (uint8)c;
    return s;
}

static void* memcpy(void* dest, const void* src, UINTN n) {
    uint8* d = dest;
    const uint8* s = src;
    for (UINTN i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static char* strncpy(char* dest, const char* src, UINTN n) {
    UINTN i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

static void fs_read_block(uint32 fs_block, void *buffer) {
    if (fs_block >= rnafs_total_blocks) return;
    uint64 lba = rnafs_partition_start_lba + ((uint64)fs_block * SECTORS_PER_BLOCK);
    read_sectors(lba, SECTORS_PER_BLOCK, buffer);
}

static void fs_write_block(uint32 fs_block, const void *buffer) {
    if (fs_block >= rnafs_total_blocks) return;
    uint64 lba = rnafs_partition_start_lba + ((uint64)fs_block * SECTORS_PER_BLOCK);
    write_sectors(lba, SECTORS_PER_BLOCK, buffer);
}

static int bitmap_test(uint32 block) {
    return (rnafs_bitmap[block / 8] >> (block % 8)) & 1;
}

static void bitmap_set(uint32 block) {
    rnafs_bitmap[block / 8] |= (1 << (block % 8));
}

static void flush_bitmap() {
    for (uint32 i = 0; i < bitmap_block_count; i++) {
        fs_write_block(1 + i, rnafs_bitmap + (i * FS_BLOCK_SIZE));
    }
}

static void flush_dir_table() {
    uint32 dir_start = 1 + bitmap_block_count;
    for (uint32 i = 0; i < dir_block_count; i++) {
        fs_write_block(dir_start + i, (uint8*)rnafs_dir_table + (i * FS_BLOCK_SIZE));
    }
}

int rnafs_alloc_blocks(uint32 count, uint32 *first_block_out) {
    uint32 run = 0;
    for (uint32 i = data_start_block; i < rnafs_total_blocks; i++) {
        if (!bitmap_test(i)) {
            run++;
            if (run == count) {
                uint32 start = i - count + 1;
                for (uint32 b = 0; b < count; b++)
                    bitmap_set(start + b);
                *first_block_out = start;
                flush_bitmap();
                return 1;
            }
        } else {
            run = 0;
        }
    }
    return 0;
}

int rnafs_find_free_dir() {
    for (uint32 i = 1; i < DIR_ENTRY_COUNT; i++)
        if (rnafs_dir_table[i].type == 0)
            return (int)i;
    return -1;
}

int rnafs_find_in_dir(uint32 parent, const char *name) {
    for (uint32 i = 0; i < DIR_ENTRY_COUNT; i++) {
        if (rnafs_dir_table[i].type != 0 &&
            rnafs_dir_table[i].parent_index == parent &&
            strcmp(rnafs_dir_table[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}

void mkfs_rnafs(uint64 partition_start_lba, uint32 partition_sector_count) {
    console_print("Formatting RNAFS partition...\n");

    uint32 total_blocks = partition_sector_count / SECTORS_PER_BLOCK;

    // Simple sizing: 1 block for bitmap, 4 blocks for directory table (v0)
    uint32 b_count = 1;
    uint32 d_count = 4;
    uint32 data_start = 1 + b_count + d_count;

    Superblock sb;
    memcpy(sb.magic, "RNAFS1", 6);
    sb.reserved = 0;
    sb.block_size = FS_BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.bitmap_start_block = 1;
    sb.bitmap_block_count = b_count;
    sb.dir_start_block = 1 + b_count;
    sb.dir_block_count = d_count;
    sb.data_start_block = data_start;
    sb.partition_start_lba = partition_start_lba;

    // Write Superblock
    memset(rnafs_block_buffer, 0, FS_BLOCK_SIZE);
    memcpy(rnafs_block_buffer, &sb, sizeof(Superblock));
    uint64 lba = partition_start_lba;
    write_sectors(lba, SECTORS_PER_BLOCK, rnafs_block_buffer);

    // Clear Bitmap
    memset(rnafs_block_buffer, 0, FS_BLOCK_SIZE);
    for (uint32 i = 0; i < b_count; i++) {
        write_sectors(lba + (1 + i) * SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, rnafs_block_buffer);
    }

    // Set metadata blocks as used in bitmap
    // This is handled in memory then written
    uint8* temp_bitmap = (uint8*)memory_alloc(FS_BLOCK_SIZE);
    memset(temp_bitmap, 0, FS_BLOCK_SIZE);
    for (uint32 i = 0; i < data_start; i++) {
        temp_bitmap[i / 8] |= (1 << (i % 8));
    }
    write_sectors(lba + SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, temp_bitmap);

    // Clear Directory Table
    memset(rnafs_block_buffer, 0, FS_BLOCK_SIZE);
    for (uint32 i = 0; i < d_count; i++) {
        write_sectors(lba + (1 + b_count + i) * SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, rnafs_block_buffer);
    }

    // Create Root Dir Entry
    DirEntry root;
    memset(&root, 0, sizeof(DirEntry));
    strncpy(root.name, "/", 31);
    root.type = 2; // dir
    root.parent_index = 0;
    write_sectors(lba + (1 + b_count) * SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, &root);

    console_print("RNAFS Format Complete.\n");
}

int rnafs_mount(uint64 partition_start_lba) {
    // Read Block 0 (Superblock)
    read_sectors(partition_start_lba, SECTORS_PER_BLOCK, rnafs_block_buffer);
    Superblock* sb = (Superblock*)rnafs_block_buffer;

    if (sb->magic[0] != 'R' || sb->magic[1] != 'N' || sb->magic[2] != 'A' ||
        sb->magic[3] != 'F' || sb->magic[4] != 'S' || sb->magic[5] != '1') {
        console_print("RNAFS: Invalid Magic. Mount Failed.\n");
        return 0;
    }

    rnafs_partition_start_lba = sb->partition_start_lba;
    rnafs_total_blocks = sb->total_blocks;
    bitmap_block_count = sb->bitmap_block_count;
    dir_block_count = sb->dir_block_count;
    data_start_block = sb->data_start_block;

    // Load Bitmap
    rnafs_bitmap = (uint8*)memory_alloc(bitmap_block_count * FS_BLOCK_SIZE);
    for (uint32 i = 0; i < bitmap_block_count; i++) {
        fs_read_block(sb->bitmap_start_block + i, rnafs_bitmap + (i * FS_BLOCK_SIZE));
    }

    // Load Directory Table
    rnafs_dir_table = (DirEntry*)memory_alloc(dir_block_count * FS_BLOCK_SIZE);
    for (uint32 i = 0; i < dir_block_count; i++) {
        fs_read_block(sb->dir_start_block + i, (uint8*)rnafs_dir_table + (i * FS_BLOCK_SIZE));
    }

    rnafs_ready = 1;
    console_print("RNAFS Mounted Successfully.\n");
    return 1;
}

int rnafs_create_file(uint32 parent, const char *name, uint32 size_bytes) {
    if (!rnafs_ready) return 0;
    uint32 blocks = (size_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

    uint32 first_block;
    if (!rnafs_alloc_blocks(blocks, &first_block))
        return 0;

    int idx = rnafs_find_free_dir();
    if (idx < 0) return 0;

    DirEntry *e = &rnafs_dir_table[idx];
    memset(e, 0, sizeof(DirEntry));

    strncpy(e->name, name, 31);
    e->type = 1;
    e->first_block = first_block;
    e->block_count = blocks;
    e->size_bytes = size_bytes;
    e->parent_index = parent;

    flush_dir_table();
    return 1;
}

int rnafs_read_file(const char *name, void *buffer, uint32 max_bytes) {
    if (!rnafs_ready) return 0;
    int idx = rnafs_find_in_dir(0, name);
    if (idx < 0) return 0;

    DirEntry *e = &rnafs_dir_table[idx];
    uint32 to_read = (e->size_bytes < max_bytes) ? e->size_bytes : max_bytes;

    uint32 bytes_read = 0;

    for (uint32 i = 0; i < e->block_count && bytes_read < to_read; i++) {
        fs_read_block(e->first_block + i, rnafs_block_buffer);

        uint32 chunk = FS_BLOCK_SIZE;
        if (bytes_read + chunk > to_read)
            chunk = to_read - bytes_read;

        memcpy((uint8*)buffer + bytes_read, rnafs_block_buffer, chunk);
        bytes_read += chunk;
    }

    return (int)bytes_read;
}

int rnafs_write_file(const char *name, const void *buffer, uint32 size_bytes) {
    if (!rnafs_ready) return 0;
    int idx = rnafs_find_in_dir(0, name);
    if (idx < 0) return 0;

    DirEntry *e = &rnafs_dir_table[idx];

    uint32 required_blocks = (size_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    if (required_blocks > e->block_count)
        return 0;

    uint32 written = 0;

    for (uint32 i = 0; i < required_blocks; i++) {
        memset(rnafs_block_buffer, 0, FS_BLOCK_SIZE);

        uint32 chunk = FS_BLOCK_SIZE;
        if (written + chunk > size_bytes)
            chunk = size_bytes - written;

        memcpy(rnafs_block_buffer, (uint8*)buffer + written, chunk);

        fs_write_block(e->first_block + i, rnafs_block_buffer);
        written += chunk;
    }

    e->size_bytes = size_bytes;
    flush_dir_table();

    return (int)written;
}

int rnafs_append_file(const char *name, const void *buffer, uint32 size_bytes) {
    if (!rnafs_ready) return 0;
    int idx = rnafs_find_in_dir(0, name);
    if (idx < 0) return 0;

    DirEntry *e = &rnafs_dir_table[idx];

    uint32 max_capacity = e->block_count * FS_BLOCK_SIZE;
    if (e->size_bytes + size_bytes > max_capacity)
        return 0;

    uint32 offset = e->size_bytes;
    uint32 written = 0;

    while (written < size_bytes) {
        uint32 block_idx = offset / FS_BLOCK_SIZE;
        uint32 block_offset = offset % FS_BLOCK_SIZE;

        fs_read_block(e->first_block + block_idx, rnafs_block_buffer);

        uint32 space = FS_BLOCK_SIZE - block_offset;
        uint32 chunk = (size_bytes - written < space) ? (size_bytes - written) : space;

        memcpy(rnafs_block_buffer + block_offset,
               (uint8*)buffer + written,
               chunk);

        fs_write_block(e->first_block + block_idx, rnafs_block_buffer);

        offset += chunk;
        written += chunk;
    }

    e->size_bytes += size_bytes;
    flush_dir_table();

    return (int)written;
}

void rnafs_ls() {
    if (!rnafs_ready) return;
    console_print("--- RNAFS File List ---\n");
    for (uint32 i = 0; i < DIR_ENTRY_COUNT; i++) {
        if (rnafs_dir_table[i].type != 0) {
            console_print(rnafs_dir_table[i].name);
            if (rnafs_dir_table[i].type == 2) console_print("/");
            console_print("\n");
        }
    }
}
