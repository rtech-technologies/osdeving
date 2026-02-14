#include "console.h"
#include "../memory/memory.h"
#include "system.h"
#include "../../unice64/io.h"
#include "font.h"
#include "../../unice64/kernel.h"

#define SERIAL_PORT 0x3F8
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

typedef struct {
    uint32 x;
    uint32 y;
    uint32 color;
    boot_params_t* params;
} console_state_t;

static console_state_t* state = NULL;

void console_ps2_init() {
    // Enable keyboard port
    outb(PS2_STATUS_PORT, 0xAE);
    io_wait();

    // Flush buffer
    int timeout = 10000;
    while (timeout-- > 0 && (inb(PS2_STATUS_PORT) & 1)) {
        inb(PS2_DATA_PORT);
        io_wait();
    }
}

void console_init() {
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x01); // 115200
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);

    console_ps2_init();

    state = (console_state_t*)memory_alloc(sizeof(console_state_t));
    if (state) {
        state->x = 0;
        state->y = 0;
        state->color = 0xFFFFFFFF;
        state->params = get_boot_params();
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
            if (line & (0x80 >> j)) draw_pixel(x + j, y + i, color);
        }
    }
}

static void scroll() {
    if (!state || !state->params || !state->params->framebuffer) return;
    uint32 lh = 16;
    uint32* fb = state->params->framebuffer;
    uint32 stride = state->params->pixels_per_scanline;
    for (uint32 y = 0; y < state->params->height - lh; y++) {
        for (uint32 x = 0; x < state->params->width; x++) {
            fb[y * stride + x] = fb[(y + lh) * stride + x];
        }
    }
    for (uint32 y = state->params->height - lh; y < state->params->height; y++) {
        for (uint32 x = 0; x < state->params->width; x++) fb[y * stride + x] = 0;
    }
    state->y -= lh;
}

void console_print(const char* str) {
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            while (!(inb(SERIAL_PORT + 5) & 0x20));
            outb(SERIAL_PORT, '\r');
            while (!(inb(SERIAL_PORT + 5) & 0x20));
            outb(SERIAL_PORT, '\n');
        } else {
            while (!(inb(SERIAL_PORT + 5) & 0x20));
            outb(SERIAL_PORT, c);
        }

        if (!state || !state->params || !state->params->framebuffer) continue;
        if (c == '\n') {
            state->x = 0;
            state->y += 16;
        } else if (c == '\r') {
            state->x = 0;
        } else if (c == '\b') {
            if (state->x >= 8) {
                state->x -= 8;
                for (int i = 0; i < 16; i++)
                    for (int j = 0; j < 8; j++) draw_pixel(state->x + j, state->y + i, 0);
            }
        } else {
            draw_char(c, state->x, state->y, state->color);
            state->x += 8;
        }
        if (state->x + 8 > state->params->width) { state->x = 0; state->y += 16; }
        if (state->y + 16 > state->params->height) scroll();
    }
}

char console_read_key() {
    // 1. Check Unified Keyboard Service (USB + PS/2)
    char key = (char)usb_get_key();
    if (key) return key;

    // 2. Check Serial Input
    if (inb(SERIAL_PORT + 5) & 1) return (char)inb(SERIAL_PORT);

    return 0;
}

void console_wait_for_key() {
    while (1) {
        if (usb_has_key()) return;
        if (inb(SERIAL_PORT + 5) & 1) return;
        __asm__ volatile("pause");
    }
}

void console_input(const char* prompt, char* buffer, size_t size) {
    if (prompt) console_print(prompt);
    size_t i = 0;
    while (1) {
        char c = console_read_key();
        if (!c) {
            __asm__ volatile("pause");
            continue;
        }

        if (c == '\r' || c == '\n') {
            buffer[i] = '\0';
            console_print("\n");
            return;
        } else if ((c == '\b' || c == 127) && i > 0) {
            i--;
            buffer[i] = '\0';
            console_print("\b \b");
        } else if (i < size - 1 && c >= 32 && c <= 126) {
            buffer[i++] = c;
            char s[2] = {c, 0};
            console_print(s);
        }
    }
}

void print(const char* str) { console_print(str); }
void wait_for_key() { console_wait_for_key(); }
char read_key() { return console_read_key(); }
void input(const char* prompt, char* buffer, size_t size) { console_input(prompt, buffer, size); }
