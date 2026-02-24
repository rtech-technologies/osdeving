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

void input(const char* prompt, char* buffer, uint64 size) {
    if (!kboot_params.SystemTable) return;

    print(prompt);

    uint64 count = 0;
    while (count < size - 1) {
        EFI_INPUT_KEY key;
        EFI_STATUS status;

        do {
            status = kboot_params.SystemTable->ConIn->ReadKeyStroke(kboot_params.SystemTable->ConIn, &key);
        } while (status != EFI_SUCCESS);

        if (key.UnicodeChar == L'\r' || key.UnicodeChar == L'\n') {
            print("\n");
            break;
        } else if (key.UnicodeChar == L'\b') {
            if (count > 0) {
                count--;
                print("\b \b");
            }
        } else if (key.UnicodeChar >= 32 && key.UnicodeChar <= 126) {
            buffer[count++] = (char)key.UnicodeChar;
            char b[2] = {(char)key.UnicodeChar, 0};
            print(b);
        }
    }
    buffer[count] = 0;
}
