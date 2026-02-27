#include "console.h"
#include "font.h"
#include "../kernel/kernel.h"
#include "../include/utils.h"

static uint32 cursor_x = 0;
static uint32 cursor_y = 0;

void console_init() {
    cursor_x = 0;
    cursor_y = 0;

    /* Clear screen */
    if (kboot_params.framebuffer) {
        for (uint32 i = 0; i < kboot_params.height * kboot_params.pixels_per_scanline; i++) {
            kboot_params.framebuffer[i] = 0x00000000;
        }
    }
}

static void scroll() {
    if (!kboot_params.framebuffer) return;

    uint32 row_size = kboot_params.pixels_per_scanline * 4; /* 4 bytes per pixel */
    uint32 total_rows = kboot_params.height;

    /* Move everything up by 10 pixels (font height + padding) */
    uint32 scroll_amount = 10;

    for (uint32 y = 0; y < total_rows - scroll_amount; y++) {
        memcpy((uint8*)kboot_params.framebuffer + y * row_size,
               (uint8*)kboot_params.framebuffer + (y + scroll_amount) * row_size,
               row_size);
    }

    /* Clear last 10 pixels */
    for (uint32 y = total_rows - scroll_amount; y < total_rows; y++) {
        for (uint32 x = 0; x < kboot_params.pixels_per_scanline; x++) {
            kboot_params.framebuffer[y * kboot_params.pixels_per_scanline + x] = 0;
        }
    }

    cursor_y -= scroll_amount;
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
            scroll();
        }
        str++;
    }
}
