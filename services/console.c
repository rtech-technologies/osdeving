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

void console_wait_for_key() {
    if (!ST || !ST->ConIn) return;
    UINTN index;
    ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &index);
}

char console_read_key() {
    if (!ST || !ST->ConIn) return 0;
    EFI_INPUT_KEY key;
    EFI_STATUS status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    if (EFI_ERROR(status)) return 0;
    return (char)key.UnicodeChar;
}

// Global implementations
void print(const char* str) {
    console_print(str);
}

void wait_for_key() {
    console_wait_for_key();
}

char read_key() {
    return console_read_key();
}
