#include "console.h"
#include "../include/system.h"
#include "../include/io.h"

#define SERIAL_PORT 0x3F8

static int serial_init_done = 0;

void console_init() {
    if (serial_init_done) return;

    // Initialize serial port
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (38400 baud)
    outb(SERIAL_PORT + 1, 0x00);    //                  (high byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    serial_init_done = 1;
}

static int is_transmit_empty() {
    return inb(SERIAL_PORT + 5) & 0x20;
}

static void serial_putc(char c) {
    while (is_transmit_empty() == 0);
    outb(SERIAL_PORT, c);
}

void console_print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');
        }
        serial_putc(*str++);
    }
}

static int serial_received() {
    return inb(SERIAL_PORT + 5) & 1;
}

char console_read_key() {
    if (serial_received()) {
        return (char)inb(SERIAL_PORT);
    }
    return 0;
}

void console_wait_for_key() {
    while (serial_received() == 0) {
        // Busy wait
        __asm__ volatile("pause");
    }
}

// Global API
void print(const char* str) {
    console_print(str);
}

void wait_for_key() {
    console_wait_for_key();
}

char read_key() {
    return console_read_key();
}
