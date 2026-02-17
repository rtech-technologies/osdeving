#include "../include/system.h"

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int program_main() {
    print("OSx2 Shell v0.1\n");
    char buffer[128];

    while (1) {
        input("> ", buffer, 128);

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting...\n");
            exit();
            break;
        } else if (strcmp(buffer, "help") == 0) {
            print("Commands: help, exit, clear\n");
        } else if (strcmp(buffer, "clear") == 0) {
            /* clear screen would be nice but not implemented as a syscall yet */
            print("Clear not implemented\n");
        } else if (buffer[0]) {
            print("Unknown command: ");
            print(buffer);
            print("\n");
        }
    }
    return 0;
}
