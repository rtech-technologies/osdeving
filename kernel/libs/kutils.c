#include "../../include/types.h"
#include "../unice64/io.h"

#define COM1 0x3F8

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

uint64 k_strlen(const char* s) {
    uint64 len = 0;
    while (*s++) len++;
    return len;
}

void k_memcpy(void* dst, const void* src, uint64 n) {
    uint8* d = (uint8*)dst;
    const uint8* s = (const uint8*)src;
    while (n--) *d++ = *s++;
}

void k_memset(void* s, int c, uint64 n) {
    uint8* p = (uint8*)s;
    while (n--) *p++ = (uint8)c;
}

void itoa(int n, char* s, int base) {
    char* p = s;
    char* p1, *p2;
    unsigned int ud = n;
    if (base == 10 && n < 0) {
        *p++ = '-';
        s++;
        ud = -n;
    }
    do {
        int remainder = ud % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= base);
    *p = 0;
    p1 = s;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
}

uint32 crc32(const void* data, uint64 len) {
    uint32 crc = 0xFFFFFFFF;
    const uint8* p = (const uint8*)data;
    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

void serial_init() {
    outb(COM1 + 1, 0x00);    // Disable all interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03);    // Set divisor to 3 (38400 baud)
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char a) {
    while (is_transmit_empty() == 0);
    outb(COM1, a);
}

void serial_print(const char* str) {
    while (*str) {
        serial_putc(*str++);
    }
}
