#include "console.h"
#include "memory.h"
#include "../include/system.h"
#include "../include/io.h"
#include "../include/font.h"
#include "../kernel/kernel.h"

#define SERIAL_PORT 0x3F8

typedef struct {
    uint32 x;
    uint32 y;
    uint32 color;
    boot_params_t* params;
} console_state_t;

static console_state_t* state = NULL;

void console_init() {
    // 1. Serial Port Initialization
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x01); // 115200
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);

    // 2. Graphics State Initialization
    state = (console_state_t*)memory_alloc(sizeof(console_state_t));
    if (state) {
        state->x = 0;
        state->y = 0;
        state->color = 0xFFFFFFFF; // White
        state->params = get_boot_params();

        // Clear screen if params are valid
        if (state->params && state->params->framebuffer) {
            for (uint32 i = 0; i < state->params->height * state->params->pixels_per_scanline; i++) {
                state->params->framebuffer[i] = 0;
            }
        }
    }
}

static void draw_pixel(uint32 x, uint32 y, uint32 color) {
    if (!state || !state->params || !state->params->framebuffer) return;
    if (x >= state->params->width || y >= state->params->height) return;
    state->params->framebuffer[y * state->params->pixels_per_scanline + x] = color;
}

static void draw_char(char c, uint32 x, uint32 y, uint32 color) {
    if (c < 32 || c > 126) return;
    const uint8* glyph = font_8x16[c - 32];
    for (int i = 0; i < 16; i++) {
        uint8 line = glyph[i];
        for (int j = 0; j < 8; j++) {
            if (line & (0x80 >> j)) {
                draw_pixel(x + j, y + i, color);
            }
        }
    }
}

static void scroll() {
    if (!state || !state->params || !state->params->framebuffer) return;
    uint32 line_height = 16;
    uint32* fb = state->params->framebuffer;
    uint32 stride = state->params->pixels_per_scanline;
    uint32 width = state->params->width;
    uint32 height = state->params->height;

    // Move everything up by 16 lines
    for (uint32 y = 0; y < height - line_height; y++) {
        for (uint32 x = 0; x < width; x++) {
            fb[y * stride + x] = fb[(y + line_height) * stride + x];
        }
    }

    // Clear the bottom 16 lines
    for (uint32 y = height - line_height; y < height; y++) {
        for (uint32 x = 0; x < width; x++) {
            fb[y * stride + x] = 0;
        }
    }
    state->y -= line_height;
}

void console_print(const char* str) {
    while (*str) {
        char c = *str++;

        // Serial mirror
        if (c == '\n') {
            while (!(inb(SERIAL_PORT + 5) & 0x20));
            outb(SERIAL_PORT, '\r');
        }
        while (!(inb(SERIAL_PORT + 5) & 0x20));
        outb(SERIAL_PORT, c);

        // Graphical output
        if (!state || !state->params || !state->params->framebuffer) continue;

        if (c == '\n') {
            state->x = 0;
            state->y += 16;
        } else if (c == '\r') {
            state->x = 0;
        } else if (c == '\b') {
            if (state->x >= 8) {
                state->x -= 8;
                // Clear the character cell
                for (int i = 0; i < 16; i++) {
                    for (int j = 0; j < 8; j++) {
                        draw_pixel(state->x + j, state->y + i, 0);
                    }
                }
            }
        } else {
            draw_char(c, state->x, state->y, state->color);
            state->x += 8;
        }

        if (state->x + 8 > state->params->width) {
            state->x = 0;
            state->y += 16;
        }

        if (state->y + 16 > state->params->height) {
            scroll();
        }
    }
}

char console_read_key() {
    if (inb(SERIAL_PORT + 5) & 1) {
        return (char)inb(SERIAL_PORT);
    }
    return 0;
}

void console_wait_for_key() {
    while (!(inb(SERIAL_PORT + 5) & 1)) {
        __asm__ volatile("pause");
    }
}

// Public API
void print(const char* str) {
    console_print(str);
}

void wait_for_key() {
    console_wait_for_key();
}

char read_key() {
    return console_read_key();
}
