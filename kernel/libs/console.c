#include "console.h"
#include "font.h"
#include "kutils.h"
#include "../unice64/kernel.h"
#include "../../include/rsl.h"

static uint32 cursor_x = 0;
static uint32 cursor_y = 0;
static uint32 fg_color = 0xFFFFFFFF;
static uint32 bg_color = 0x00000000;

void console_init() {
    cursor_x = 0;
    cursor_y = 0;
    #ifdef CONFIG_EMERALD_MODE
    fg_color = 0x00FF88;
    #else
    fg_color = CONFIG_FG_COLOR;
    #endif
    bg_color = CONFIG_BG_COLOR;
    if (kboot_params.framebuffer && (uint64)kboot_params.framebuffer != 0) {
        for (uint32 i = 0; i < kboot_params.height * kboot_params.pixels_per_scanline; i++) {
            kboot_params.framebuffer[i] = bg_color;
        }

        /* [BASE] Emerald Signature: Single pixel at (0,0) */
        kboot_params.framebuffer[0] = 0x00FF88;
        serial_print("EFI: RSL_PRINT_START (Emerald Signature confirmed)\n");
    }
    print("Console: Freestanding Graphics Driver initialized.\n");
}

void console_clear() {
    console_init();
}

void console_set_color(uint32 fg, uint32 bg) {
    fg_color = fg;
    bg_color = bg;
}

static void scroll() {
    if (!kboot_params.framebuffer || (uint64)kboot_params.framebuffer == 0) return;
    uint32 row_size = kboot_params.pixels_per_scanline * 4;
    uint32 total_rows = kboot_params.height;
    uint32 scroll_amount = CONFIG_SCROLL_SPEED;
    for (uint32 y = 0; y < total_rows - scroll_amount; y++) {
        memcpy((uint8*)kboot_params.framebuffer + y * row_size,
               (uint8*)kboot_params.framebuffer + (y + scroll_amount) * row_size,
               row_size);
    }
    for (uint32 y = total_rows - scroll_amount; y < total_rows; y++) {
        for (uint32 x = 0; x < kboot_params.pixels_per_scanline; x++) {
            kboot_params.framebuffer[y * kboot_params.pixels_per_scanline + x] = bg_color;
        }
    }
    cursor_y -= scroll_amount;
}

static void draw_char(char c, uint32 x, uint32 y, uint32 color) {
    if (!kboot_params.framebuffer || (uint64)kboot_params.framebuffer == 0) return;
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
    if (!kboot_params.framebuffer || (uint64)kboot_params.framebuffer == 0) return;
    while (*str) {
        if (*str == '\n') {
            cursor_x = 0;
            cursor_y += 10;
        } else if (*str == '\r') {
            cursor_x = 0;
        } else if (*str == '\b') {
            if (cursor_x >= 8) {
                cursor_x -= 8;
                draw_char(' ', cursor_x, cursor_y, bg_color);
            }
        } else {
            draw_char(*str, cursor_x, cursor_y, fg_color);
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
