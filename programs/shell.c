#include "../include/system.h"

void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int program_main() {
    print("Shell started!\n");
    char buffer[128];
    UINTN pos = 0;

    while (1) {
        print("> ");
        pos = 0;
        while (1) {
            wait_for_key();
            char c = read_key();

            if (c == '\r' || c == '\n') {
                print("\n");
                buffer[pos] = 0;
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    print("\b \b");
                }
            } else if (c >= ' ' && pos < 127) {
                buffer[pos++] = c;
                char s[2] = {c, 0};
                print(s);
            }
        }

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting shell...\n");
            exit();
            return 0;
        } else if (strcmp(buffer, "return") == 0) {
            print("Returning from shell without exit...\n");
            return 0;
        } else if (strcmp(buffer, "help") == 0) {
            print("Available commands: help, exit, return\n");
        } else if (pos > 0) {
            print("Unknown command: ");
            print(buffer);
            print("\n");
        }
    }
    return 0;
}
