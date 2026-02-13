#include <efi.h>
#include <efilib.h>
#include "console.h"
#include "../include/system.h"

void console_init() {
    if (ST && ST->ConOut) {
        ST->ConOut->ClearScreen(ST->ConOut);
    }
}

void console_print(const char* str) {
    if (!ST || !ST->ConOut) return;

    CHAR16 buffer[256];
    int i = 0;

    while (*str) {
        if (*str == '\n') {
            if (i > 253) { // Need space for \r\n and null
                buffer[i] = 0;
                ST->ConOut->OutputString(ST->ConOut, buffer);
                i = 0;
            }
            buffer[i++] = '\r';
            buffer[i++] = '\n';
            str++;
        } else {
            if (i > 254) {
                buffer[i] = 0;
                ST->ConOut->OutputString(ST->ConOut, buffer);
                i = 0;
            }
            buffer[i++] = (CHAR16)*str++;
        }
    }
    buffer[i] = 0;
    if (i > 0) {
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
