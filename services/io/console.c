#include "console.h"
#include "../../kernel/kernel.h"

void console_init() {
    if (kboot_params.SystemTable) {
        kboot_params.SystemTable->ConOut->ClearScreen(kboot_params.SystemTable->ConOut);
    }
}

void print(const char* str) {
    if (!kboot_params.SystemTable) return;

    CHAR16 buf[2];
    buf[1] = 0;

    while (*str) {
        if (*str == '\n') {
            buf[0] = L'\r';
            kboot_params.SystemTable->ConOut->OutputString(kboot_params.SystemTable->ConOut, buf);
            buf[0] = L'\n';
            kboot_params.SystemTable->ConOut->OutputString(kboot_params.SystemTable->ConOut, buf);
        } else {
            buf[0] = (CHAR16)*str;
            kboot_params.SystemTable->ConOut->OutputString(kboot_params.SystemTable->ConOut, buf);
        }
        str++;
    }
}
