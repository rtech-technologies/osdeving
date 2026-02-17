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

void input_init() {
    /* PS/2 keyboard init could go here if needed */
}

char read_key() {
    uint8 status;
    uint8 scancode;

    /* Poll for data */
    while (1) {
        __asm__ volatile("inb $0x64, %0" : "=a"(status));
        if (status & 0x01) {
            __asm__ volatile("inb $0x60, %0" : "=a"(scancode));
            if (!(scancode & 0x80)) {
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
                /* Visual backspace */
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
