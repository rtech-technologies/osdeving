#include "input.h"
#include "console.h"
#include "../../include/types.h"

static const char scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

static void outb(uint16 port, uint8 val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static uint8 inb(uint16 port) {
    uint8 ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void wait_input() {
    while (inb(0x64) & 0x02);
}

static void wait_output() {
    while (!(inb(0x64) & 0x01));
}

void input_init() {
    /* PS/2 Controller Initialization */

    /* 1. Disable devices */
    wait_input();
    outb(0x64, 0xAD); /* Disable P1 */
    wait_input();
    outb(0x64, 0xA7); /* Disable P2 */

    /* 2. Flush output buffer */
    while (inb(0x64) & 0x01) inb(0x60);

    /* 3. Set Controller Configuration Byte */
    wait_input();
    outb(0x64, 0x20); /* Read CCB */
    wait_output();
    uint8 ccb = inb(0x60);
    ccb |= 0x01; /* Enable Port 1 interrupt (optional for polling but good practice) */
    ccb &= ~0x10; /* Clear Port 1 clock disable */
    wait_input();
    outb(0x64, 0x60); /* Write CCB */
    wait_input();
    outb(0x60, ccb);

    /* 4. Controller Self-Test */
    wait_input();
    outb(0x64, 0xAA);
    wait_output();
    if (inb(0x60) != 0x55) {
        print("PS/2: Controller self-test failed\n");
    }

    /* 5. Enable devices */
    wait_input();
    outb(0x64, 0xAE); /* Enable P1 */

    /* 6. Reset Keyboard & Enable Scanning */
    wait_input();
    outb(0x60, 0xFF); /* Reset */
    wait_output();
    inb(0x60); /* ACK/PASS */

    wait_input();
    outb(0x60, 0xF4); /* Enable Scanning - MANDATORY */
    wait_output();
    inb(0x60); /* ACK */
}

char read_key() {
    uint8 status;
    uint8 scancode;

    while (1) {
        status = inb(0x64);
        if (status & 0x01) {
            scancode = inb(0x60);
            if (!(scancode & 0x80) && scancode < 128) {
                return scancode_map[scancode];
            }
        }
    }
}

void input(const char* prompt, char* buffer, uint64 size) {
    if (prompt) print(prompt);

    uint64 i = 0;
    while (i < size - 1) {
        char c = read_key();
        if (c == '\n') {
            print("\n");
            break;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                print("\b \b");
            }
        } else if (c >= 32 && c <= 126) {
            buffer[i++] = c;
            char s[2] = {c, 0};
            print(s);
        }
    }
    buffer[i] = 0;
}
