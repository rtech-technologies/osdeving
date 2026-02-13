#include <efi.h>
#include <efilib.h>
#include "console.h"
#include "../include/system.h"

void console_init() {
    // Basic initialization if needed. Clear screen is a good choice for v0.
    if (ST && ST->ConOut) {
        ST->ConOut->ClearScreen(ST->ConOut);
    }
}

void console_print(const char* str) {
    if (!ST || !ST->ConOut) return;

    CHAR16 buffer[2];
    buffer[1] = 0;

    while (*str) {
        if (*str == '\n') {
            buffer[0] = '\r';
            ST->ConOut->OutputString(ST->ConOut, buffer);
        }
        buffer[0] = *str++;
        ST->ConOut->OutputString(ST->ConOut, buffer);
    }
}

// Global print implementation
void print(const char* str) {
    console_print(str);
}
