#include "../include/system.h"

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int program_main() {
    print("Shell started\n");
    print("Type 'exit' to quit kernel\n");

    char buffer[128];
    while (1) {
        input("> ", buffer, 128);

        if (strcmp(buffer, "exit") == 0) {
            print("Exiting shell...\n");
            exit();
            break;
        } else if (buffer[0] != 0) {
            print("Unknown command: ");
            print(buffer);
            print("\n");
        }
    }

    return 0;
}
