#include "rnafs.h"
#include "../io/disk.h"
#include "../io/console.h"
#include "../mem/memory.h"

/* Category 3: size checks */
typedef char static_assertion_sizeof_Superblock[(sizeof(Superblock) == 44) ? 1 : -1];
typedef char static_assertion_sizeof_DirEntry[(sizeof(DirEntry) == 64) ? 1 : -1];

static uint64 current_partition_lba = 0;
static Superblock mounted_sb;
static int rnafs_ready = 0;

#ifdef FS_DISABLED
#define IS_FS_DISABLED 1
#else
#define IS_FS_DISABLED 0
#endif

/* Utility functions */
static int memcmp(const void* s1, const void* s2, uint64 n) {
    const uint8* p1 = s1;
    const uint8* p2 = s2;
    for (uint64 i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = dst;
    const uint8* s = src;
    for (uint64 i = 0; i < n; i++) d[i] = s[i];
}

static void memset(void* s, uint8 c, uint64 n) {
    uint8* p = s;
    for (uint64 i = 0; i < n; i++) p[i] = c;
}

static void strncpy(char* dst, const char* src, uint64 n) {
    uint64 i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
}

/* Category 5: block -> sector conversion */
static void read_block(uint32 block_idx, void* buffer) {
    uint64 lba = current_partition_lba + ((uint64)block_idx * SECTORS_PER_BLOCK);
    read_sectors(lba, SECTORS_PER_BLOCK, buffer);
}

static void write_block(uint32 block_idx, const void* buffer) {
    uint64 lba = current_partition_lba + ((uint64)block_idx * SECTORS_PER_BLOCK);
    write_sectors(lba, SECTORS_PER_BLOCK, buffer);
}

int rnafs_mount(uint64 partition_start_lba) {
    if (IS_FS_DISABLED) {
        rnafs_ready = 1;
        return 1;
    }

    current_partition_lba = partition_start_lba;

    uint8 buffer[FS_BLOCK_SIZE];
    read_block(0, buffer);
    memcpy(&mounted_sb, buffer, sizeof(Superblock));

    if (memcmp(mounted_sb.magic, "RNAFS1", 6) != 0) {
        /* User requested: do NOT format on boot */
        return 0;
    }

    rnafs_ready = 1;
    return 1;
}

int rnafs_read_file(const char* name, void* buffer, uint32 max_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return -1;

    uint32 dir_block_idx = mounted_sb.dir_start_block;
    DirEntry* entries = (DirEntry*)alloc(mounted_sb.dir_block_count * FS_BLOCK_SIZE);
    for (uint32 i = 0; i < mounted_sb.dir_block_count; i++) {
        read_block(dir_block_idx + i, (uint8*)entries + (i * FS_BLOCK_SIZE));
    }

    uint32 entry_count = (mounted_sb.dir_block_count * FS_BLOCK_SIZE) / sizeof(DirEntry);
    int found_idx = -1;
    for (uint32 i = 0; i < entry_count; i++) {
        if (entries[i].type == 1 && strcmp(entries[i].name, name) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) return -1;

    DirEntry* e = &entries[found_idx];
    uint32 to_read = (e->size_bytes < max_bytes) ? e->size_bytes : max_bytes;

    uint32 start_block = e->first_block;
    uint32 blocks_to_read = (to_read + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

    for (uint32 i = 0; i < blocks_to_read; i++) {
        uint8 block_buf[FS_BLOCK_SIZE];
        read_block(start_block + i, block_buf);
        uint32 chunk = (to_read > FS_BLOCK_SIZE) ? FS_BLOCK_SIZE : to_read;
        memcpy((uint8*)buffer + (i * FS_BLOCK_SIZE), block_buf, chunk);
        to_read -= chunk;
        if (to_read == 0) break;
    }

    return (int)e->size_bytes;
}

int rnafs_create_file(uint32 parent, const char* name, uint32 size_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return 0;

    uint32 dir_block_idx = mounted_sb.dir_start_block;
    DirEntry* entries = (DirEntry*)alloc(mounted_sb.dir_block_count * FS_BLOCK_SIZE);

    for (uint32 i = 0; i < mounted_sb.dir_block_count; i++) {
        read_block(dir_block_idx + i, (uint8*)entries + (i * FS_BLOCK_SIZE));
    }

    uint32 entry_count = (mounted_sb.dir_block_count * FS_BLOCK_SIZE) / sizeof(DirEntry);
    int free_idx = -1;
    for (uint32 i = 0; i < entry_count; i++) {
        if (entries[i].type == 0) {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1) return 0;

    /* Allocation logic: find next free block using bitmap */
    uint8* bitmap = (uint8*)alloc(mounted_sb.bitmap_block_count * FS_BLOCK_SIZE);
    for (uint32 i = 0; i < mounted_sb.bitmap_block_count; i++) {
        read_block(mounted_sb.bitmap_start_block + i, bitmap + (i * FS_BLOCK_SIZE));
    }

    uint32 blocks_needed = (size_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    uint32 found_start = 0;
    uint32 count = 0;

    for (uint32 i = mounted_sb.data_start_block; i < mounted_sb.total_blocks; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            if (count == 0) found_start = i;
            count++;
            if (count == blocks_needed) break;
        } else {
            count = 0;
        }
    }

    if (count < blocks_needed) return 0;

    for (uint32 i = found_start; i < found_start + blocks_needed; i++) {
        bitmap[i / 8] |= (1 << (i % 8));
    }

    for (uint32 i = 0; i < mounted_sb.bitmap_block_count; i++) {
        write_block(mounted_sb.bitmap_start_block + i, bitmap + (i * FS_BLOCK_SIZE));
    }

    DirEntry* e = &entries[free_idx];
    memset(e, 0, sizeof(DirEntry));
    strncpy(e->name, name, 31);
    e->type = 1;
    e->size_bytes = size_bytes;
    e->parent_index = parent;
    e->first_block = found_start;
    e->block_count = blocks_needed;

    uint32 block_to_write = dir_block_idx + (free_idx * sizeof(DirEntry)) / FS_BLOCK_SIZE;
    write_block(block_to_write, (uint8*)entries + (block_to_write - dir_block_idx) * FS_BLOCK_SIZE);

    return 1;
}

int rnafs_write_file(const char* name, const void* buffer, uint32 size_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return 0;

    uint32 dir_block_idx = mounted_sb.dir_start_block;
    DirEntry* entries = (DirEntry*)alloc(mounted_sb.dir_block_count * FS_BLOCK_SIZE);
    for (uint32 i = 0; i < mounted_sb.dir_block_count; i++) {
        read_block(dir_block_idx + i, (uint8*)entries + (i * FS_BLOCK_SIZE));
    }

    uint32 entry_count = (mounted_sb.dir_block_count * FS_BLOCK_SIZE) / sizeof(DirEntry);
    int found_idx = -1;
    for (uint32 i = 0; i < entry_count; i++) {
        if (entries[i].type == 1 && strcmp(entries[i].name, name) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) return 0;

    DirEntry* e = &entries[found_idx];
    uint32 required_blocks = (size_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    if (required_blocks > e->block_count) return 0;

    uint32 start_block = e->first_block;
    uint32 remaining = size_bytes;
    for (uint32 i = 0; i < required_blocks; i++) {
        uint8 block_buf[FS_BLOCK_SIZE];
        memset(block_buf, 0, FS_BLOCK_SIZE);
        uint32 chunk = (remaining > FS_BLOCK_SIZE) ? FS_BLOCK_SIZE : remaining;
        memcpy(block_buf, (uint8*)buffer + (i * FS_BLOCK_SIZE), chunk);
        write_block(start_block + i, block_buf);
        remaining -= chunk;
    }

    e->size_bytes = size_bytes;
    uint32 block_to_write = dir_block_idx + (found_idx * sizeof(DirEntry)) / FS_BLOCK_SIZE;
    write_block(block_to_write, (uint8*)entries + (block_to_write - dir_block_idx) * FS_BLOCK_SIZE);

    return 1;
}
