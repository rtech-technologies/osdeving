#include "console.h"
#include "font.h"
#include "../kernel/kernel.h"

static uint32 cursor_x = 0;
static uint32 cursor_y = 0;

void console_init() {
    cursor_x = 0;
    cursor_y = 0;

    if (!kboot_params.framebuffer) {
        return; /* Framebuffer not available yet */
    }

    /* Clear screen */
    if (kboot_params.width > 0 && kboot_params.height > 0) {
        uint32 clear_color = 0x00000000;
        for (uint32 i = 0; i < kboot_params.height * kboot_params.pixels_per_scanline; i++) {
            kboot_params.framebuffer[i] = clear_color;
        }
    }
}

static void draw_char(char c, uint32 x, uint32 y, uint32 color) {
    if (!kboot_params.framebuffer) return;
    if ((uint8)c >= 128) return;

    const uint8* glyph = font8x8_basic[(uint8)c];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (glyph[i] & (1 << j)) {
                uint32 py = y + i;
                uint32 px = x + j;
                if (px < kboot_params.width && py < kboot_params.height) {
                    kboot_params.framebuffer[py * kboot_params.pixels_per_scanline + px] = color;
                }
            }
        }
    }
}

void print(const char* str) {
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += 10;
        } else if (*str == '\r') {
            cursor_x = 0;
        } else if (*str == '\b') {
            if (cursor_x >= 8) {
                cursor_x -= 8;
                draw_char(' ', cursor_x, cursor_y, 0);
            }
        } else {
            draw_char(*str, cursor_x, cursor_y, 0xFFFFFFFF);
            cursor_x += 8;
            if (cursor_x + 8 > kboot_params.width) {
                cursor_x = 0;
                cursor_y += 10;
            }
        }

        if (cursor_y + 10 > kboot_params.height) {
            /* Scroling would be nice, but for v0 we just wrap or clear */
            console_init();
        }
        str++;
    }
}
