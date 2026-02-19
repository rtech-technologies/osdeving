#include "rnafs.h"
#include "../io/disk.h"
#include "../io/console.h"
#include "../mem/memory.h"

/* Category 3: size checks */
typedef char static_assertion_sizeof_Superblock[(sizeof(Superblock) == 44) ? 1 : -1];
typedef char static_assertion_sizeof_DirEntry[(sizeof(DirEntry) == 64) ? 1 : -1];

#define MAX_BITMAP_BLOCKS 1
#define MAX_DIR_BLOCKS    4
#define ENTRIES_PER_BLOCK (FS_BLOCK_SIZE / sizeof(DirEntry))

static uint64 current_partition_lba = 0;
static Superblock mounted_sb;
static int rnafs_ready = 0;

/* Fixed buffers for metadata - No dynamic allocation after mount */
static uint8 rnafs_bitmap[MAX_BITMAP_BLOCKS * FS_BLOCK_SIZE];
static DirEntry rnafs_dir_table[MAX_DIR_BLOCKS * ENTRIES_PER_BLOCK];

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

/* Block I/O */
static void read_block(uint32 block_idx, void* buffer) {
    uint64 lba = current_partition_lba + ((uint64)block_idx * SECTORS_PER_BLOCK);
    read_sectors(lba, SECTORS_PER_BLOCK, buffer);
}

static void write_block(uint32 block_idx, const void* buffer) {
    uint64 lba = current_partition_lba + ((uint64)block_idx * SECTORS_PER_BLOCK);
    write_sectors(lba, SECTORS_PER_BLOCK, buffer);
}

/* Internal Allocator */
static int rnafs_alloc_blocks(uint32 count, uint32* start_out) {
    uint32 run = 0;
    uint32 start = 0;
    uint32 total_bits = mounted_sb.total_blocks;

    for (uint32 i = mounted_sb.data_start_block; i < total_bits; i++) {
        if (!(rnafs_bitmap[i / 8] & (1 << (i % 8)))) {
            if (run == 0) start = i;
            run++;
            if (run == count) {
                *start_out = start;
                return 1;
            }
        } else {
            run = 0;
        }
    }
    return 0;
}

static void rnafs_flush_metadata() {
    /* Write Superblock */
    uint8 sb_buf[FS_BLOCK_SIZE];
    memset(sb_buf, 0, FS_BLOCK_SIZE);
    memcpy(sb_buf, &mounted_sb, sizeof(Superblock));
    write_block(0, sb_buf);

    /* Write Bitmap */
    for (uint32 i = 0; i < mounted_sb.bitmap_block_count && i < MAX_BITMAP_BLOCKS; i++) {
        write_block(mounted_sb.bitmap_start_block + i, rnafs_bitmap + (i * FS_BLOCK_SIZE));
    }

    /* Write Dir Table */
    for (uint32 i = 0; i < mounted_sb.dir_block_count && i < MAX_DIR_BLOCKS; i++) {
        write_block(mounted_sb.dir_start_block + i, (uint8*)rnafs_dir_table + (i * FS_BLOCK_SIZE));
    }
}

void mkfs_rnafs(uint64 partition_start_lba, uint32 total_sectors) {
    current_partition_lba = partition_start_lba;
    uint32 total_blocks = total_sectors / SECTORS_PER_BLOCK;

    memset(&mounted_sb, 0, sizeof(Superblock));
    memcpy(mounted_sb.magic, "RNAFS1", 6);
    mounted_sb.block_size = FS_BLOCK_SIZE;
    mounted_sb.total_blocks = total_blocks;
    mounted_sb.bitmap_start_block = 1;
    mounted_sb.bitmap_block_count = 1;
    mounted_sb.dir_start_block = 2;
    mounted_sb.dir_block_count = 4;
    mounted_sb.data_start_block = 6;
    mounted_sb.partition_start_lba = partition_start_lba;

    memset(rnafs_bitmap, 0, sizeof(rnafs_bitmap));
    /* Mark metadata blocks as used in bitmap */
    for (uint32 i = 0; i < mounted_sb.data_start_block; i++) {
        rnafs_bitmap[i / 8] |= (1 << (i % 8));
    }

    memset(rnafs_dir_table, 0, sizeof(rnafs_dir_table));
    /* Initialize Root directory entry at index 0 */
    strncpy(rnafs_dir_table[0].name, "/", 31);
    rnafs_dir_table[0].type = 2; /* Directory */
    rnafs_dir_table[0].parent_index = 0;

    rnafs_flush_metadata();
    rnafs_ready = 1;
    print("RNAFS: Partition formatted successfully.\n");
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
        return 0;
    }

    /* Load bitmap into fixed buffer */
    uint32 b_blocks = (mounted_sb.bitmap_block_count > MAX_BITMAP_BLOCKS) ? MAX_BITMAP_BLOCKS : mounted_sb.bitmap_block_count;
    for (uint32 i = 0; i < b_blocks; i++) {
        read_block(mounted_sb.bitmap_start_block + i, rnafs_bitmap + (i * FS_BLOCK_SIZE));
    }

    /* Load directory table into fixed buffer */
    uint32 d_blocks = (mounted_sb.dir_block_count > MAX_DIR_BLOCKS) ? MAX_DIR_BLOCKS : mounted_sb.dir_block_count;
    for (uint32 i = 0; i < d_blocks; i++) {
        read_block(mounted_sb.dir_start_block + i, (uint8*)rnafs_dir_table + (i * FS_BLOCK_SIZE));
    }

    rnafs_ready = 1;
    return 1;
}

int rnafs_read_file(const char* name, void* buffer, uint32 max_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return -1;

    uint32 entry_count = MAX_DIR_BLOCKS * ENTRIES_PER_BLOCK;
    int found_idx = -1;
    for (uint32 i = 0; i < entry_count; i++) {
        if (rnafs_dir_table[i].type == 1 && strcmp(rnafs_dir_table[i].name, name) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) return -1;

    DirEntry* e = &rnafs_dir_table[found_idx];
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

    uint32 entry_count = MAX_DIR_BLOCKS * ENTRIES_PER_BLOCK;
    int free_idx = -1;
    for (uint32 i = 1; i < entry_count; i++) { /* Skip root */
        if (rnafs_dir_table[i].type == 0) {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1) return 0;

    uint32 blocks_needed = (size_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    if (blocks_needed == 0) blocks_needed = 1;

    uint32 found_start;
    if (!rnafs_alloc_blocks(blocks_needed, &found_start)) return 0;

    /* Mark blocks as used in bitmap */
    for (uint32 i = found_start; i < found_start + blocks_needed; i++) {
        rnafs_bitmap[i / 8] |= (1 << (i % 8));
    }

    /* Write bitmap blocks back to disk */
    for (uint32 i = 0; i < mounted_sb.bitmap_block_count && i < MAX_BITMAP_BLOCKS; i++) {
        write_block(mounted_sb.bitmap_start_block + i, rnafs_bitmap + (i * FS_BLOCK_SIZE));
    }

    DirEntry* e = &rnafs_dir_table[free_idx];
    memset(e, 0, sizeof(DirEntry));
    strncpy(e->name, name, 31);
    e->type = 1;
    e->size_bytes = size_bytes;
    e->parent_index = parent;
    e->first_block = found_start;
    e->block_count = blocks_needed;

    /* Write back the directory block containing this entry */
    uint32 block_in_table = free_idx / ENTRIES_PER_BLOCK;
    write_block(mounted_sb.dir_start_block + block_in_table, (uint8*)rnafs_dir_table + (block_in_table * FS_BLOCK_SIZE));

    return 1;
}

int rnafs_write_file(const char* name, const void* buffer, uint32 size_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return 0;

    uint32 entry_count = MAX_DIR_BLOCKS * ENTRIES_PER_BLOCK;
    int found_idx = -1;
    for (uint32 i = 0; i < entry_count; i++) {
        if (rnafs_dir_table[i].type == 1 && strcmp(rnafs_dir_table[i].name, name) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) return 0;

    DirEntry* e = &rnafs_dir_table[found_idx];
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
    uint32 block_in_table = found_idx / ENTRIES_PER_BLOCK;
    write_block(mounted_sb.dir_start_block + block_in_table, (uint8*)rnafs_dir_table + (block_in_table * FS_BLOCK_SIZE));

    return 1;
}

void rnafs_ls() {
    if (!rnafs_ready) return;
    print("RNAFS File List:\n");
    uint32 entry_count = MAX_DIR_BLOCKS * ENTRIES_PER_BLOCK;
    for (uint32 i = 0; i < entry_count; i++) {
        if (rnafs_dir_table[i].type != 0) {
            print(rnafs_dir_table[i].name);
            if (rnafs_dir_table[i].type == 2) print("/");
            print("\n");
        }
    }
}
