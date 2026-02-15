#ifndef IO_H
#define IO_H

#include "../../include/types.h"

static inline void outb(uint16 port, uint8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8 inb(uint16 port) {
    uint8 ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16 port, uint32 val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32 inl(uint16 port) {
    uint32 ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

static inline uint32 mmio_read32(uint64 addr) {
    return *(volatile uint32*)addr;
}

static inline void mmio_write32(uint64 addr, uint32 val) {
    *(volatile uint32*)addr = val;
}

static inline uint64 mmio_read64(uint64 addr) {
    return *(volatile uint64*)addr;
}

static inline void mmio_write64(uint64 addr, uint64 val) {
    *(volatile uint64*)addr = val;
}

static inline uint32 pci_read_config_32(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
    uint32 address;
    uint32 lbus  = (uint32)bus;
    uint32 lslot = (uint32)slot;
    uint32 lfunc = (uint32)func;
    uint32 tmp = 0;

    address = (uint32)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32)0x80000000));

    outl(0xCF8, address);
    tmp = inl(0xCFC);
    return tmp;
}

static inline void pci_write_config_32(uint8 bus, uint8 slot, uint8 func, uint8 offset, uint32 val) {
    uint32 address;
    uint32 lbus  = (uint32)bus;
    uint32 lslot = (uint32)slot;
    uint32 lfunc = (uint32)func;

    address = (uint32)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32)0x80000000));

    outl(0xCF8, address);
    outl(0xCFC, val);
}

#endif
