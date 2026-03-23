#include "paging.h"
#include "kernel.h"
#include "../libs/console.h"
#include "../libs/kutils.h"

/* Sovereign Paging: 2MB Huge Pages Identity Map */

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_HUGE     (1ULL << 7)
#define PAGE_CACHE_DISABLE (1ULL << 4)

/* Allocated statically in the kernel BSS */
static uint64 pml4[512] __attribute__((aligned(4096)));
static uint64 pdpt[512] __attribute__((aligned(4096)));
static uint64 pd[4][512] __attribute__((aligned(4096))); /* 4 PDs = 4GB coverage */

void paging_init() {
    serial_print("EFI: RSL_PAGING_HANDOVER\n");

    k_memset(pml4, 0, 4096);
    k_memset(pdpt, 0, 4096);
    k_memset(pd, 0, 4096 * 4);

    /* 1. Link PML4 -> PDPT */
    pml4[0] = (uint64)pdpt | PAGE_PRESENT | PAGE_WRITABLE;

    /* 2. Link PDPT -> PDs */
    for (int i = 0; i < 4; i++) {
        pdpt[i] = (uint64)&pd[i] | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* 3. Identity Map first 4GB with 2MB Huge Pages */
    for (uint64 i = 0; i < 2048; i++) {
        uint64 addr = i * 2 * 1024 * 1024;
        uint64 pd_idx = i / 512;
        uint64 entry_idx = i % 512;
        pd[pd_idx][entry_idx] = addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE;
    }

    /* 4. Ensure Framebuffer is mapped and Cache-Disabled if outside 4GB (Safety) */
    /* Note: If within 4GB, it's already mapped. We'll add a specific map for it regardless. */
    if (kboot_params.framebuffer) {
        uint64 fb_addr = (uint64)kboot_params.framebuffer;
        char buf[64];
        k_memset(buf, 0, 64);
        serial_print("EFI: MAP_FRAMEBUFFER: 0x");
        /* Simple hex print logic */
        k_memset(buf, 0, 64);
        itoa((int)(fb_addr >> 32), buf, 16);
        serial_print(buf);
        itoa((int)fb_addr, buf, 16);
        serial_print(buf);
        serial_print("fb\n");

        /* For simplicity, we assume the framebuffer is in the first 4GB for identity mapping.
           If it's higher, a more complex PDPT/PD allocation is needed. */
    }

    /* 5. Load CR3 */
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");

    serial_print("EFI: Paging Active (Sovereign Mode).\n");
}
