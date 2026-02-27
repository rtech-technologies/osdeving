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
};

static const char shift_map[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '\"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
};

static int shift_pressed = 0;

static void ps2_wait_write() {
    while (inb(0x64) & 2);
}

static void ps2_wait_read() {
    while (!(inb(0x64) & 1));
}

void input_init() {
    shift_pressed = 0;

    /* 1. Disable devices */
    ps2_wait_write();
    outb(0x64, 0xAD);
    ps2_wait_write();
    outb(0x64, 0xA7);

    /* 2. Flush buffer */
    while (inb(0x64) & 1) inb(0x60);

    /* 3. Set Controller Config */
    ps2_wait_write();
    outb(0x64, 0x20);
    ps2_wait_read();
    uint8 ccb = inb(0x60);
    ccb |= 1;
    ccb &= ~0x40;
    ps2_wait_write();
    outb(0x64, 0x60);
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
    if (inb(0x60) != 0xFA) return;
    ps2_wait_read();
    if (inb(0x60) != 0xAA) return;

    /* 7. Enable Scanning */
    ps2_wait_write();
    outb(0x60, 0xF4);
    ps2_wait_read();
    inb(0x60);
}

static char get_char() {
    while (1) {
        if (inb(0x64) & 1) {
            uint8 scancode = inb(0x60);

            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
                continue;
            }
            if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = 0;
                continue;
            }

            if (!(scancode & 0x80)) {
                if (scancode < 128) {
                    return shift_pressed ? shift_map[scancode] : scancode_map[scancode];
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
