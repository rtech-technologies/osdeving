#include "../../include/types.h"

typedef struct {
    uint16 limit_low;
    uint16 base_low;
    uint8  base_mid;
    uint8  access;
    uint8  granularity;
    uint8  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16 limit;
    uint64 base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt[3];
static gdt_ptr_t gdt_ptr;

void gdt_init() {
    /* Null descriptor */
    gdt[0] = (gdt_entry_t){0, 0, 0, 0, 0, 0};

    /* 64-bit Code segment: Access 0x9A (present, ring 0, code, readable), Gran 0x20 (long mode) */
    gdt[1] = (gdt_entry_t){0, 0, 0, 0x9A, 0x20, 0};

    /* 64-bit Data segment: Access 0x92 (present, ring 0, data, writable) */
    gdt[2] = (gdt_entry_t){0, 0, 0, 0x92, 0, 0};

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64)&gdt;

    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr));
}
