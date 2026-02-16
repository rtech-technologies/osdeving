#include "console.h"

void console_init() {
    if (ST_PTR && ST_PTR->ConOut) {
        ST_PTR->ConOut->ClearScreen(ST_PTR->ConOut);
    }
}

void print(const char* str) {
    if (!ST_PTR || !ST_PTR->ConOut) return;

    CHAR16 wstr[256];
    UINTN i = 0;
    while (str[i] && i < 255) {
        wstr[i] = (CHAR16)str[i];
        i++;
    }
    wstr[i] = 0;
    ST_PTR->ConOut->OutputString(ST_PTR->ConOut, wstr);
}
