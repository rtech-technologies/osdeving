#include "rnafs.h"
#include "disk.h"
#include "console.h"
#include "memory.h"

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
    return *(uint8*)s1 - *(uint8*)s2;
}

static void memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = dst;
    const uint8* s = src;
    for (uint64 i = 0; i < n; i++) d[i] = s[i];
}


/* Category 5: block -> sector conversion */
static void read_block(uint32 block_idx, void* buffer) {
    uint64 lba = current_partition_lba + ((uint64)block_idx * SECTORS_PER_BLOCK);
    read_sectors(lba, SECTORS_PER_BLOCK, buffer);
}


/* Category 9: Superblock Verification debug step */
void rnafs_debug_dump_superblock(uint64 lba) {
    uint8 buffer[512];
    if (!read_sectors(lba, 1, buffer)) {
        print("Debug: Failed to read block 0\n");
        return;
    }

    print("Superblock Hex Dump (64 bytes):\n");
    for (int i = 0; i < 64; i++) {
        /* Simple hex print (v0) */
        char hex[3];
        const char* chars = "0123456789ABCDEF";
        hex[0] = chars[(buffer[i] >> 4) & 0xF];
        hex[1] = chars[buffer[i] & 0xF];
        hex[2] = ' ';
        char s[4] = {hex[0], hex[1], hex[2], 0};
        print(s);
        if ((i + 1) % 16 == 0) print("\n");
    }
}

int rnafs_mount(uint64 partition_start_lba) {
    if (IS_FS_DISABLED) {
        rnafs_ready = 1;
        return 1;
    }

    /* Category 6: Mount must trust the runtime argument */
    current_partition_lba = partition_start_lba;

    uint8 buffer[FS_BLOCK_SIZE];
    read_block(0, buffer);
    memcpy(&mounted_sb, buffer, sizeof(Superblock));

    /* Category 4: Never use strcmp for magic */
    if (memcmp(mounted_sb.magic, "RNAFS1", 6) != 0) {
        print("RNAFS: Invalid magic\n");
        return 0;
    }

    rnafs_ready = 1;
    return 1;
}

int rnafs_read_file(const char* name, void* buffer, uint32 max_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return -1;

    /* Load directory blocks */
    uint32 dir_block = mounted_sb.dir_start_block;
    DirEntry* entries = (DirEntry*)alloc(mounted_sb.dir_block_count * FS_BLOCK_SIZE);
    for (uint32 i = 0; i < mounted_sb.dir_block_count; i++) {
        read_block(dir_block + i, (uint8*)entries + (i * FS_BLOCK_SIZE));
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

    /* Contiguous read */
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

    return 1;
}

/* Category 7: Directory loading behavior - Read-Modify-Write */
int rnafs_create_file(uint32 parent, const char* name, uint32 size_bytes) {
    if (!rnafs_ready || IS_FS_DISABLED) return 0;

    /* 1. Find free entry and free blocks (v0 simplified: just find first free) */
    /* ... skipped implementation for brevity in v0 but logic should be there ... */
    return 0; /* Stub for now */
}
