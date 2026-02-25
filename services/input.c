#include "input.h"
#include "console.h"
#include "../kernel/io.h"

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

static void ps2_wait_write() {
    while (inb(0x64) & 2);
}

static void ps2_wait_read() {
    while (!(inb(0x64) & 1));
}

void input_init() {
    /* 10-step initialization sequence (simplified but robust) */

    /* 1. Disable devices */
    ps2_wait_write();
    outb(0x64, 0xAD); /* Disable P1 */
    ps2_wait_write();
    outb(0x64, 0xA7); /* Disable P2 */

    /* 2. Flush buffer */
    while (inb(0x64) & 1) inb(0x60);

    /* 3. Set Controller Config */
    ps2_wait_write();
    outb(0x64, 0x20); /* Read CCB */
    ps2_wait_read();
    uint8 ccb = inb(0x60);
    ccb |= 1; /* Enable P1 interrupts (though we poll) */
    ccb &= ~0x40; /* Disable translation */
    ps2_wait_write();
    outb(0x64, 0x60); /* Write CCB */
    ps2_wait_write();
    outb(0x60, ccb);

    /* 4. Controller self-test */
    ps2_wait_write();
    outb(0x64, 0xAA);
    ps2_wait_read();
    if (inb(0x60) != 0x55) return;

    /* 5. Enable P1 */
    ps2_wait_write();
    outb(0x64, 0xAE);

    /* 6. Reset Keyboard */
    ps2_wait_write();
    outb(0x60, 0xFF);
    ps2_wait_read();
    if (inb(0x60) != 0xFA) return; /* ACK */
    ps2_wait_read();
    if (inb(0x60) != 0xAA) return; /* Success */

    /* 7. Enable Scanning */
    ps2_wait_write();
    outb(0x60, 0xF4);
    ps2_wait_read();
    inb(0x60); /* Flush ACK */
}

static char get_char() {
    while (1) {
        if (inb(0x64) & 1) {
            uint8 scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                if (scancode < 128) {
                    return scancode_map[scancode];
                }
            }
        }
        __asm__ volatile("pause");
    }
}

void input(const char* prompt, char* buffer, uint64 size) {
    print(prompt);
    uint64 i = 0;
    while (i < size - 1) {
        char c = get_char();
        if (c == '\n') {
            print("\n");
            break;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                print("\b \b");
            }
        } else if (c > 0) {
            buffer[i++] = c;
            char s[2] = {c, 0};
            print(s);
        }
    }
    buffer[i] = 0;
}
